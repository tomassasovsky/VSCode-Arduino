#include <HID-Project.h>

#define DEBOUNCE 10  // how many ms to debounce, 5+ ms is usually plenty

byte buttons[] = {3, 2, 6};
#define NUMBUTTONS sizeof(buttons)

//track if a button is just pressed, just released, or 'currently pressed' 
byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];
byte previous_keystate[NUMBUTTONS], current_keystate[NUMBUTTONS];
byte lastPressed;

int lastState;
int state;

#define dataPin 4
#define clockPin 5

void setup(){
  for (byte i = 0; i < NUMBUTTONS; i++) pinMode(buttons[i], INPUT_PULLUP);
  pinMode(dataPin, INPUT_PULLUP);
  pinMode(clockPin, INPUT_PULLUP);
  lastState = digitalRead(clockPin);
  Consumer.begin();
  BootKeyboard.begin();
}

void loop(){
  switch(pressedButton()) {
    case 0: // mic button, shortcut:
      BootKeyboard.press(KEY_LEFT_CTRL);
      BootKeyboard.write('m');
      BootKeyboard.release(KEY_LEFT_CTRL);
      break;
    case 1: // camera button, shortcut:
      BootKeyboard.press(KEY_LEFT_CTRL);
      BootKeyboard.press(KEY_LEFT_SHIFT);
      BootKeyboard.write('k');
      BootKeyboard.release(KEY_LEFT_CTRL);
      BootKeyboard.release(KEY_LEFT_SHIFT);
      break;
    case 2:
      Consumer.write(MEDIA_PLAY_PAUSE);
      break;
  }
  rotaryEncoder();
}
     
void checkButtons() {
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
byte pressedButton() {
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

void rotaryEncoder() {
  state = digitalRead(clockPin);
  if (state != lastState) {
    if (digitalRead(dataPin) != state) 
      Consumer.write(MEDIA_VOLUME_UP);
    else 
      Consumer.write(MEDIA_VOLUME_DOWN);
  }
  lastState = state;
}