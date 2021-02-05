#include <RBDdimmer.h> //dimmer library

#define zerocross  2
#define fanOut  4 //FAN OUT
#define lightsOut 3 //LIGHTS OUT
#define NUMBUTTONS sizeof(buttons)
#define tube 6
#define Fan 0
#define Lights 1
#define Tube 2
byte buttons[] = {A0, A1};

dimmerLamp dimmer1(fanOut);// FAN OUT
dimmerLamp dimmer2(lightsOut);// LIGHTS OUT
byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];
byte previous_keystate[NUMBUTTONS], current_keystate[NUMBUTTONS];

int State[] = {0, 0, 0};

long time = 0;    //the last time the output pin was toggled
long debounceTime = 10;    //the debounce time, increase if the output flickers (was 150)

int fanSpeed;

char Received = ' ';
bool canPrint = true;

void setup() {
  Serial.begin(9600);
  dimmer1.begin(NORMAL_MODE, OFF);
  dimmer2.begin(NORMAL_MODE, OFF);
  for (byte i = 0; i < NUMBUTTONS; i++) pinMode(buttons[i], INPUT_PULLUP);
  pinMode(tube, OUTPUT);
}

void loop() {
    if (Serial.available() > 0) {
      Received = Serial.read();
    }
    if (Received == 'F') State[Fan]++, Received = ' ', canPrint = true;
    if (Received == 'L') State[Lights]++, Received = ' ';
    if (Received == 'T') State[Tube]++, Received = ' ';

    byte pressedButton = thisSwitch_justPressed();
    switch(pressedButton) {
        case 0: 
            State[Lights]++;
        break;
        case 1:
             State[Fan]++;
        break;
    }
    switch(State[Fan]){
        case 1:
            dimmer1.setState(ON);
            dimmer1.setPower(55);
            fanSpeed = 20;
        break;
        case 2:
            dimmer1.setPower(60);
            fanSpeed = 40;
        break;
        case 3:
            dimmer1.setPower(67);
            fanSpeed = 60;
        break;
        case 4:
            dimmer1.setPower(75);
            fanSpeed = 80;
        break;
        case 5:
            dimmer1.setPower(94);
            fanSpeed = 100;
        break;
        default:
            dimmer1.setPower(0);
            dimmer1.setState(OFF);
            if (canPrint){
                Serial.println("Fan is OFF");
                canPrint = false;
            }
            fanSpeed = 0;
            State[Fan] = 0;
        break;        
    }

    if (State[Fan] && canPrint){
        Serial.print("Fan Speed:");
        printSpace(dimmer1.getPower()); 
        Serial.print(fanSpeed);
        Serial.println("%");
        canPrint = false;    
    }
 
    switch(State[Lights]){
        case 1:
            dimmer2.setState(ON);
            dimmer2.setPower(96);
        break;
        default:
            dimmer2.setPower(0);
            dimmer2.setState(OFF);
            State[Lights] = 0;
        break;        
    }
 
    switch(State[Tube]){
        case 1:
            digitalWrite(tube, HIGH);
        break;
        default:
            digitalWrite(tube, LOW);
            State[Tube] = 0;
        break;        
    }

}

void printSpace(int val){
  if ((val / 100) == 0) Serial.print(" ");
  if ((val / 10) == 0) Serial.print(" ");
}

void check_switches(){
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  byte index;
  if (millis() < lasttime) lasttime = millis();
  if ((lasttime + debounceTime) > millis()) return;
  // ok we have waited debounceTime milliseconds, lets reset the timer
  lasttime = millis();
  for (index = 0; index < NUMBUTTONS; index++) {
    justpressed[index] = 0;       //when we start, we clear out the "just" indicators
    justreleased[index] = 0;
    currentstate[index] = digitalRead(buttons[index]);   //read the button
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
        // just pressed
        justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
        justreleased[index] = 1; // just released
      }
      pressed[index] = !currentstate[index];  //remember, digital HIGH means NOT pressed
    }
    previousstate[index] = currentstate[index]; //keep a running tally of the buttons
  }
}
 
byte thisSwitch_justPressed() {
  byte thisSwitch = 255;
  check_switches();  //check the switches &amp; get the current state
  for (byte i = 0; i < NUMBUTTONS; i++) {
    current_keystate[i]=justpressed[i];
    if (current_keystate[i] != previous_keystate[i]) {
      if (current_keystate[i]) thisSwitch=i;
    }
    previous_keystate[i]=current_keystate[i];
  }
  return thisSwitch;
}