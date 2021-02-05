#include <FastLED.h>    //library for controlling the LEDs
#include <EEPROM.h>    //Library to store the potentiometer values
#define DEBOUNCE 10  // how many ms to debounce, 5+ ms is usually plenty

byte buttons[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, A2};
#define NUMBUTTONS sizeof(buttons)

//track if a button is just pressed, just released, or 'currently pressed' 
byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];
byte previous_keystate[NUMBUTTONS], current_keystate[NUMBUTTONS];
byte lastPressed;

#define pinLEDs 2
#define dataPin A0
#define clockPin A1
#define swPin A2
#define qtyLEDs 19
#define typeOfLEDs WS2812B    //type of LEDs i'm using
#define nonRingLEDs 7
#define modeLED 12
#define Tr1LED 13
#define Tr2LED 14
#define Tr3LED 15
#define Tr4LED 16
#define clearLED 17
#define X2LED 18
CRGB LEDs[qtyLEDs]; 

int lastState;
int state;
unsigned int counterLEDs = 0;
byte vol[4];

unsigned long timeClear = 0;
unsigned long timeVolume = 0;
unsigned long timeSleepMode = 0;
unsigned long lastLEDsMillis = 0;
#define doublePressClearTime 750

char State[4] = {'E', 'E', 'E', 'E'};

//the next tags are to make it simpler to select a track when used in a function
#define TR1 0
#define TR2 1
#define TR3 2
#define TR4 3

byte ringPosition = 0;  //the led ring has 12 leds. This variable is to set the position in which the first lit led is during the animation.
int setRingColour = 0;    //yellow is 35/50 = 45, red is 0, green is 96
#define ringSpeed  54.6875    //46.875 62.5

/*
when Rec/Play is pressed in Play Mode, every track gets played, no matter if they are empty or not.
because of this, we need to keep an even tighter tracking of each of the track's states
so if Rec/Play is pressed in Play Mode and, for example, the track 3 is empty, the corresponding variable (Tr3PlayedWRecPlay) turns true
likewise, when the button Stop is pressed, every track gets muted, so every one of these variables turn false.
*/
bool PlayedWRecPlay[4] = {false, false, false, false};
bool PressedInStop[4] = {false, false, false, false};    //when a track is pressed-in-stop, it gets focuslocked

/*
Selected track, to record on it without having to press the individual track.
Plus, if you record using the RecPlayButton, you will also overdub, unlike pressing the track button, 
which would only record and then play (no overdub).
*/

byte selectedTrack = TR1;    //this variable is to keep constant track of the selected track
byte firstRecordedTrack;    //the firstRecordedTrack variable is for when you start recording but the time is not set in Mobius, so that you cant start recording on another track until the first is played.
bool firstRecording = true;    //this track is only true when the pedal has not been used to record yet.
bool stopMode = false;
bool stopModeUsed = false;
bool playMode = false;    //false = Rec mode, true = Play Mode
bool sleepMode = false;
bool startLEDsDone = false;
bool doublePressClear = false;
bool doublePressPlay = false;

#define midichannel 0x90

void setup() {
  Serial.begin(31250);    //to use only the Arduino UNO (Atmega16u2). You must upload another bootloader to use it as a native MIDI-USB device.
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LEDs
  for (byte i = 0; i < 4; i++) vol[i] = EEPROM.read(i);
  for (byte i = 0; i < NUMBUTTONS; i++) pinMode(buttons[i], INPUT_PULLUP);
  pinMode(dataPin, INPUT_PULLUP);
  pinMode(clockPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  selectedTrack = min(selectedTrack, 3);
  lastState = digitalRead(clockPin);
  sendNote(0x1F);    //resets the pedal
  muteAllInputsButSelected();
}

void loop() {
  if ((State[TR1] != 'E' || State[TR2] != 'E' || State[TR3] != 'E' || State[TR4] != 'E') && !stopMode) ringLEDs();    //the led ring spins when the pedal has been used.
  if (State[TR1] == 'E' && State[TR2] == 'E' && State[TR3] == 'E' && State[TR4] == 'E' && !firstRecording) reset();   //reset the pedal when every track is empty but it has been used (for example, when the only track playing is Tr1 and you clear it, it resets the whole pedal)
  if (digitalRead(11) == HIGH) doublePressClear = false; 

  if (PressedInStop[TR1] || PressedInStop[TR2] || PressedInStop[TR3] || PressedInStop[TR4]) stopModeUsed = true;
  else stopModeUsed = false;
  
  if (State[selectedTrack] == 'R' && firstRecording) firstRecordedTrack = selectedTrack;
  else firstRecordedTrack = 5;

  if (!startLEDsDone) startLEDs();
  else setLEDs();

  if (millis() - timeVolume > 3000){
    if (firstRecording){
      sendAllVolumes();
      muteAllInputsButSelected();
      timeVolume = millis();
    }
  }

  if (millis() - timeSleepMode > 180000) {
    if (firstRecording && State[TR1] == 'E' && State[TR2] == 'E' && State[TR3] == 'E' && State[TR4] == 'E') {
      sleepMode = true;
      timeSleepMode = millis();
    }
  }

  switch(pressedButton()) {
    case 0:
      if (sleepMode) resetSleepMode();
      else if (!playMode) RecPlay();   //if the Rec/Play button is pressed
      else playRecPlay();
      doublePressClear = false;
      doublePressPlay = true;
    break;
    case 1:
      if (sleepMode) resetSleepMode();
      else if (!playMode) recModeStop();
      else playModeStop();
      doublePressClear = false;
      doublePressPlay = false;
      lastPressed = 1;
    break;
    case 2:
      if (sleepMode) resetSleepMode();
      else if (canChangeModeOrClearTrack()) undoButton();
      doublePressClear = false;
      doublePressPlay = false;
      lastPressed = 2;
    break;
    case 3:
      if (sleepMode) resetSleepMode();
      else modeButton();
      doublePressClear = false;
      doublePressPlay = false;
      lastPressed = 3;
    break;
    case 4:
      if (sleepMode) resetSleepMode();
      else pressedTrack(TR1);
      doublePressClear = false;
      lastPressed = 4;
    break;
    case 5:
      if (sleepMode) resetSleepMode();
      else pressedTrack(TR2);
      doublePressClear = false;
      lastPressed = 5;
    break;
    case 6:
      if (sleepMode) resetSleepMode();
      else pressedTrack(TR3);
      doublePressClear = false;
      lastPressed = 6;
    break;
    case 7:
      if (sleepMode) resetSleepMode();
      else pressedTrack(TR4);
      doublePressClear = false;
      lastPressed = 7;
    break;
    case 9:
      if (sleepMode) resetSleepMode();
      else X2Button();
      doublePressClear = false;
      doublePressPlay = false;
      lastPressed = 9;
    break;
    case 10:
      if (sleepMode) resetSleepMode();
      else if (canChangeModeOrClearTrack() && !stopMode) nextTrack();
      doublePressClear = false;
      doublePressPlay = false;
      lastPressed = 10;
    break;
    case 11:
      lastPressed = 11;
    break;
  }
  clearButton();
  rotaryEncoder();
}

void checkButtons(){
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static unsigned long lasttime;
  byte index;
  if (millis() < lasttime) lasttime = millis();
  if ((lasttime + DEBOUNCE) > millis()) return;
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
  for (index = 0; index < NUMBUTTONS; index++) {
    justpressed[index] = 0;       //when we start, we clear out the "just" indicators
    justreleased[index] = 0;
    currentstate[index] = digitalRead(buttons[index]);   //read the button
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
        // just pressed
        justpressed[index] = 1;
      } else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
        justreleased[index] = 1; // just released
      }
      pressed[index] = !currentstate[index];  //remember, digital HIGH means NOT pressed
    }
    previousstate[index] = currentstate[index]; //keep a running tally of the buttons
  }
} 
byte pressedButton(){
  byte thisSwitch = 255;
  checkButtons();  //check the switches &amp; get the current state
  for (byte i = 0; i < NUMBUTTONS; i++) {
    current_keystate[i]=justpressed[i];
    if (current_keystate[i] != previous_keystate[i]) {
      if (current_keystate[i]) thisSwitch=i;
    }
    previous_keystate[i]=current_keystate[i];
  }
  return thisSwitch;
}
void sendNote(int note){
  Serial.write(midichannel);    //sends the notes in the selected channel
  Serial.write(note);    //sends note
  Serial.write(0x45);    //medium velocity = Note ON
}
void setLEDs(){
  if (digitalRead(11) == LOW){    //turn ON the Clear LED when the button is pressed and the pedal has been used
    if (!firstRecording){
      if (!stopModeUsed && !stopMode) {
        if (State[selectedTrack] == 'P' || State[selectedTrack] == 'E') LEDs[clearLED] = CRGB(0,0,255);
        else if (State[selectedTrack] != 'P' && State[selectedTrack] != 'E') LEDs[clearLED] = CRGB(255,0,0);
      } else LEDs[clearLED] = CRGB(255,0,0);
    } else LEDs[clearLED] = CRGB(255,0,0);
  } else LEDs[clearLED] = CRGB(0,0,0);    //turn OFF the Clear LED when the button is NOT pressed
  
  if (digitalRead(12) == LOW) {
    if (!stopModeUsed && !stopMode) {
      if (State[selectedTrack] != 'R' && State[selectedTrack] != 'O' && !firstRecording) LEDs[X2LED] = CRGB(0,0,255);    //turn ON the X2 LED when the button is pressed and the pedal has been used
      else LEDs[X2LED] = CRGB(255,0,0);
    } else LEDs[X2LED] = CRGB(255,0,0);
  } else LEDs[X2LED] = CRGB(0,0,0);    //turn OFF the X2 LED when the button is NOT pressed
  
  if (!sleepMode) {
    if (!playMode) LEDs[modeLED] = CRGB(255,0,0);    //when we are in Record mode, the LED is red
    else LEDs[modeLED] = CRGB(0,255,0);    //when we are in Play mode, the LED is green
  } else LEDs[modeLED] = CRGB(0,0,0);    //when we are in sleep mode

  if (State[TR1] == 'R' || State[TR1] == 'O') LEDs[Tr1LED] = CRGB(255,0,0);    //when track 1 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR1] == 'P') LEDs[Tr1LED] = CRGB(0,255,0);    //when track 1 is playing, turn on it's LED, colour green
  else if (State[TR1] == 'M' || State[TR1] == 'E') LEDs[Tr1LED] = CRGB(0,0,0);    //when track 1 is muted or empty, turn off it's LED
  
  if (State[TR2] == 'R' || State[TR2] == 'O') LEDs[Tr2LED] = CRGB(255,0,0);    //when track 2 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR2] == 'P') LEDs[Tr2LED] = CRGB(0,255,0);    //when track 2 is playing, turn on it's LED, colour green
  else if (State[TR2] == 'M' || State[TR2] == 'E') LEDs[Tr2LED] = CRGB(0,0,0);    //when track 2 is muted or empty, turn off it's LED
  
  if (State[TR3] == 'R' || State[TR3] == 'O') LEDs[Tr3LED] = CRGB(255,0,0);    //when track 3 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR3] == 'P') LEDs[Tr3LED] = CRGB(0,255,0);    //when track 3 is playing, turn on it's LED, colour green
  else if (State[TR3] == 'M' || State[TR3] == 'E') LEDs[Tr3LED] = CRGB(0,0,0);    //when track 3 is muted or empty, turn off it's LED
  
  if (State[TR4] == 'R' || State[TR4] == 'O') LEDs[Tr4LED] = CRGB(255,0,0);    //when track 4 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR4] == 'P') LEDs[Tr4LED] = CRGB(0,255,0);    //when track 4 is playing, turn on it's LED, colour green
  else if (State[TR4] == 'M' || State[TR4] == 'E') LEDs[Tr4LED] = CRGB(0,0,0);    //when track 4 is muted or empty, turn off it's LED

  if (stopModeUsed) {
    for (byte i = 0; i <= 3; i++) {
      if (State[i] == 'M' && PressedInStop[i]) {
        if (i == TR1) LEDs[Tr1LED] = CRGB(0,255,0);
        if (i == TR2) LEDs[Tr2LED] = CRGB(0,255,0);
        if (i == TR3) LEDs[Tr3LED] = CRGB(0,255,0);
        if (i == TR4) LEDs[Tr4LED] = CRGB(0,255,0);
      }
    }
  }

  //the setRingColour variable sets the LED Ring's colour. 0 is red, 60 is yellow(ish), 96 is green.
  if ((State[TR1] == 'R' || State[TR2] == 'R' || State[TR3] == 'R' || State[TR4] == 'R') && firstRecording) setRingColour = 0;    //sets the led ring to red (recording)
  else if ((State[TR1] == 'R' || State[TR2] == 'R' || State[TR3] == 'R' || State[TR4] == 'R') && !firstRecording) setRingColour = 60;    //sets the led ring to yellow (overdubbing)
  else if (State[TR1] == 'O' || State[TR2] == 'O' || State[TR3] == 'O' || State[TR4] == 'O') setRingColour = 60;    //sets the led ring to yellow (overdubbing)
  else if (((State[TR1] == 'M' || State[TR1] == 'E') && (State[TR2] == 'M' || State[TR2] == 'E') && (State[TR3] == 'M' || State[TR3] == 'E') && (State[TR4] == 'M' || State[TR4] == 'E')) && !firstRecording && !PlayedWRecPlay[TR1] && !PlayedWRecPlay[TR2] && !PlayedWRecPlay[TR3] && !PlayedWRecPlay[TR4]) stopMode = true;    //stop the spinning ring animation when all tracks are muted
  else if (firstRecording) setRingColour = 0;
  else setRingColour = 96;    //sets the led ring to green (playing)
  
  FastLED.show();    //updates the led states
}
void reset(){    //function to reset the pedal
  sendNote(0x1F);    //sends note that resets Mobius
  selectedTrack = TR1;    //selects the 1rst track
  muteAllInputsButSelected();
  sendAllVolumes();
  resetSleepMode();
  //empties the tracks:
  for (byte i = 0; i < 4; i++){
    State[i] = 'E';
    PressedInStop[i] = false;
    PlayedWRecPlay[i] = false;
  }
  doublePressClear = false;
  stopModeUsed = false;
  stopMode = false;
  firstRecording = true;    //sets the variable firstRecording to true
  playMode = false;    //into record mode
  ringPosition = 0;    //resets the ring position
  startLEDs();    //animation to show the pedal has been reset
  startLEDsDone = false;
}
void startLEDs(){    //animation to show the pedal has been reset
  if (millis() - lastLEDsMillis > 150){
    if (counterLEDs % 2 == 0 && counterLEDs < 6) fill_solid(LEDs, qtyLEDs, CRGB::Red), counterLEDs++;
    else if (counterLEDs % 2 != 0 && counterLEDs < 6) FastLED.clear(), counterLEDs++;
    FastLED.show();
    if (counterLEDs == 6) startLEDsDone = true, counterLEDs = 0;
    lastLEDsMillis = millis();
  }
  setRingColour = 0;    //sets the led ring colour to green
}
void ringLEDs(){    //function that makes the spinning animation for the led ring
  FastLED.setBrightness(255);
  EVERY_N_MILLISECONDS(ringSpeed){    //this gets an entire spin in 3/4 of a second (0.75s) Half a second would be 31.25, a second would be 62.5
    fadeToBlackBy(LEDs, qtyLEDs - nonRingLEDs, 70); //Dims the LEDs by 64/256 (1/4) and thus sets the trail's length. Also set limit to the ring LEDs quantity and exclude the mode and track ones (qtyLEDs-7)
    LEDs[ringPosition] = CHSV(setRingColour, 255, 255);    //Sets the LED's hue according to the potentiometer rotation    
    ringPosition++;    //Shifts all LEDs one step in the currently active direction    
    if (ringPosition == qtyLEDs - nonRingLEDs) ringPosition = 0;    //If one end is reached, reset the position to loop around
    FastLED.show();    //Finally, display all LED's data (illuminate the LED ring)
  }
}
void sendAllVolumes(){
  for (byte i = 0; i <= TR4; i++){
    Serial.write(176);    //176 = CC Command
    Serial.write(i);    // Which value: if the selected Track is 1, the value sent will be 1; If it's 2, the value 2 will be sent and so on.
    Serial.write(vol[i]);
  }
}
void sendVolume(){
  Serial.write(176);    //176 = CC Command
  Serial.write(selectedTrack);    // Which value: if the selected Track is 1, the value sent will be 1; If it's 2, the value 2 will be sent and so on.
  Serial.write(vol[selectedTrack]);
  EEPROM.write(selectedTrack, vol[selectedTrack]);
}
void muteAllInputsButSelected(){
  byte notMutedInput = 0;
  if (selectedTrack == TR1) notMutedInput = 5;
  else if (selectedTrack == TR2) notMutedInput = 6;
  else if (selectedTrack == TR3) notMutedInput = 7;
  else if (selectedTrack == TR4) notMutedInput = 8;
  for (byte i = 5; i <= 8; i++) {
    if (i != notMutedInput) {
      Serial.write(176);
      Serial.write(i);
      Serial.write(0);
    }
  }
}
void RecPlay(){
  muteAllInputsButSelected();
  sendVolume();
  sendNote(0x27);    //send the note
  stopMode = false;
  if (firstRecording){
    if (State[selectedTrack] != 'R') State[selectedTrack] = 'R';    //if the track is either empty or playing, record
    else State[selectedTrack] = 'O', firstRecording = false;    //if the track is recording, overdub
  } else {
    if (State[selectedTrack] == 'P' || State[selectedTrack] == 'E'){
      for (byte i = 0; i < 4; i++){
        if (State[i] == 'R' || State[i] == 'O') State[i] = 'P'; //if another track is being recorded, stop and play it
      }
      State[selectedTrack] = 'O';   //if it's playing or empty, overdub
    } else if (State[selectedTrack] == 'R') {
      State[selectedTrack] = 'P';    
      if (selectedTrack != TR4) selectedTrack++;    //if the track is recording, play it and select the next track
      else selectedTrack = TR1;
    }
    else if (State[selectedTrack] == 'O') {
      State[selectedTrack] = 'P';
      if (selectedTrack != TR4) selectedTrack++;    //if the track is overdubbing, play and select the next track
      else selectedTrack = TR1;
    } 
    else if (State[selectedTrack] == 'M') State[selectedTrack] = 'R';    //if the track is muted, record
  }
}
void playRecPlay(){
  sendNote(0x31);    //send the note
  if (stopMode) ringPosition = 0;    //if we are in stop mode, make the ring spin from the position 0
  if (!stopModeUsed) {    //if stop Mode hasn't been used, just play every track that isn't empty: 
    for (byte i = 0; i < 4; i++) {
      if (State[i] != 'E') State[i] = 'P';
      else if (State[i] == 'E') PlayedWRecPlay[i] = true;
    }
  } else if (!doublePressPlay && stopModeUsed) {    //if stop mode has been used and the previous pressed button isn't RecPlay
    if (!PressedInStop[TR1] && !PressedInStop[TR2] && !PressedInStop[TR3] && !PressedInStop[TR4]){ //if none of the tracks has been pressed in stop, play every track that isn't empty:
      for (byte i = 0; i < 4; i++){
        if (State[i] != 'E') State[i] = 'P';
        else if (State[i] == 'E') PlayedWRecPlay[i] = true;
      }
    } else {    //if a track has been pressed and it isn't empty, play it
      for (byte i = 0; i < 4; i++){
        if (PressedInStop[i]) State[i] = 'P';
      }
      doublePressPlay = true;
    }
  } else if (doublePressPlay && stopModeUsed) {    //if the previous pressed button is RecPlay, play every track that is not empty:
    if (checkPressedInStop(TR1) && checkPressedInStop(TR2) && checkPressedInStop(TR3) && checkPressedInStop(TR4)) {
      for (byte i = 0; i < 4; i++){
        if (PressedInStop[i]) State[i] = 'P';
      }
    } else {
      for (byte i = 0; i < 4; i++){
        if (State[i] != 'E') State[i] = 'P';
        else if (State[i] == 'E') PlayedWRecPlay[i] = true;
      }
    }
  }
  stopMode = false;
}
void pressedTrack(byte track){
  if (!playMode) {
    if ((track == firstRecordedTrack) || firstRecordedTrack == 5){
      selectedTrack = track;    //select the track 1
      muteAllInputsButSelected();
      if (track == TR1) sendNote(0x23);    //send the note
      else if (track == TR2) sendNote(0x24);    //send the note
      else if (track == TR3) sendNote(0x25);    //send the note
      else if (track == TR4) sendNote(0x26);    //send the note
      stopMode = false;
      if (State[track] == 'P' || State[track] == 'E' || State[track] == 'M'){    //if it's playing, empty or muted, record it
        for (byte i = 0; i < 4; i++){
          if (State[i] == 'R' || State[i] == 'O') State[i] = 'P'; //if another track is being recorded, stop and play it
        }
        State[track] = 'R';
      } else if (State[track] == 'R') State[track] = 'P', firstRecording = false;    //if the track is recording, play it
      else if (State[track] == 'O') State[track] = 'P', firstRecording = false;    //if the track is overdubbing, play it
    }
  } else {
    if (State[track] != 'E') {
      if (track == TR1) sendNote(0x2C);
      else if (track == TR2) sendNote(0x2D);
      else if (track == TR3) sendNote(0x2E);
      else if (track == TR4) sendNote(0x2F);
      if (stopMode){
        PressedInStop[track] = !PressedInStop[track];    //toggle between the track being pressed in stop mode and not
        selectedTrack = track;    //select the track
      } else {
        if (PressedInStop[track] && State[track] == 'P') {
          State[track] = 'M';
        }
        else if (PressedInStop[track] && State[track] == 'M') {
          doublePressPlay = true;
          State[track] = 'P';
          stopModeUsed = false;
          for (byte i = 0; i < 4; i++) PressedInStop[i] = false;
        }
        else if (!PressedInStop[track]){    //if it wasn't pressed in stop
          if (State[track] == 'P') State[track] = 'M';    //and it is playing, mute it
          else if (State[track] == 'M'){    //else if it is muted, play it
            //reset everything in stopMode:
            State[track] = 'P';
            stopModeUsed = false;
            for (byte i = 0; i < 4; i++) PressedInStop[i] = false;
          }
        }
      }
      selectedTrack = track;    //select the track
    }
  }
}
void recModeStop(){
  if (canStopTrack()) {
    if (State[selectedTrack] != 'O' && State[selectedTrack] != 'R' && State[selectedTrack] != 'E'){
      sendNote(0x28);    //send the note
      State[selectedTrack] = 'M';
    }
  }
}
void playModeStop(){
  sendNote(0x2A);    //send the note
  for (byte i = 0; i < 4; i++){
    if (State[i] != 'E') State[i] = 'M'; //if another track is being recorded, stop and play it
    PlayedWRecPlay[i] = false;
  }
  stopMode = true;    //make the stopMode variable and mode true
}
void undoButton(){
  if (!playMode && lastPressed != 1 && lastPressed != 11) sendNote(0x22);    //send a note that undoes the last thing it did
}
void modeButton(){
  if (canChangeModeOrClearTrack()){ //if the mode Button is pressed and mode can be changed
    if (playMode == false){    //if the current mode is Record Mode
      sendNote(0x29);    //send a note so that the pedal knows we've change modes
      playMode = true;    //enter play mode
    }else{
      sendNote(0x2B);
      playMode = false;    //entering rec mode
    }
    //if any of the tracks is recording when pressing the button, play them:
    for (byte i = 0; i < 4; i++){
      if (State[i] != 'M' && State[i] != 'E') State[i] = 'P';
      PressedInStop[i] = false;
    }
    // reset the focuslock function:
    stopModeUsed = false;
  }
}
bool canStopTrack(){
  if ((State[selectedTrack] == 'R' || State[selectedTrack] == 'O') && firstRecording) return false;
  else return true;
}
bool canChangeModeOrClearTrack(){
  if (firstRecordedTrack == 5) return true;
  else return false;
}
void X2Button(){
  if (!stopModeUsed && !stopMode) {
    if ((State[selectedTrack] == 'P' || State[selectedTrack] == 'M' || State[selectedTrack] == 'E') && !firstRecording) {
      sendNote(0x20);
      for (byte i = 0; i < 4; i++){
        if (State[i] == 'M') State[i] = 'P'; //play a track if it is muted
        else if (State[i] == 'E') PlayedWRecPlay[i] = "true";
      }
    }
  }
}
void clearButton(){
  if (millis() - timeClear > doublePressClearTime){    //debounce
    if (digitalRead(11) == LOW){
      if (!doublePressClear){
        if (canChangeModeOrClearTrack()){    //if we can clear the selected track, do it
          if (!stopModeUsed && !stopMode) {
            sendNote(0x1E);
            doublePressClear = true;    //activate the function that resets everything if we press the Clear button again
            if (State[selectedTrack] == 'P' || State[selectedTrack] == 'O' || (State[selectedTrack] == 'R' && !firstRecording)) {
              State[selectedTrack] = 'E';
              PlayedWRecPlay[selectedTrack] = false;
              PressedInStop[selectedTrack] = false;
            }
          } else doublePressClear = true;
        } else if (canChangeModeOrClearTrack() && stopMode) doublePressClear = true;
        else if (!canChangeModeOrClearTrack()) reset();
        timeClear = millis();
      } else reset();    //reset everything when Clear is pressed twice
      resetSleepMode();
    } 
  }
}
void rotaryEncoder(){
  state = digitalRead(clockPin);
  if (state != lastState){
    if (digitalRead(dataPin) != state){
      if (vol[selectedTrack] < 127){
        vol[selectedTrack]++;
      }
    }
    else {
      if(vol[selectedTrack] > 0){
        vol[selectedTrack]--;
      }
    }
    resetSleepMode();
    sendVolume();
  }
  lastState = state;
}
void nextTrack(){
  bool wasRecording = false;
  for (byte i = 0; i < 4; i++){
    if (State[i] == 'R' || State[i] == 'O'){
      State[i] = 'P'; //if another track is being recorded, stop and play it
      wasRecording = true;
    } 
  }
  if (selectedTrack < TR4) selectedTrack++;
  else selectedTrack = TR1;
  if (wasRecording) State[selectedTrack] = 'O';
  sendNote(0x32);
}
void resetSleepMode(){
  if (sleepMode) sleepMode = false;
  timeSleepMode = millis();
}
bool checkPressedInStop(byte track){
  if (State[track] != 'E') {
    if (PressedInStop[track]) {
      if (State[track] == 'M') return true;
      else return false;
    } else return true;
  } else return true;
}