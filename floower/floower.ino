#include <Adafruit_NeoPixel.h>
#include <Adafruit_TiCoServo.h>
#include "SoftPWM.h"

#define NEOPIXEL_PIN 3
#define TOUCH_SENSOR_PIN 2

#define SERVO_PIN 9
//#define SERVO_OPEN 1750
#define SERVO_OPEN 1650
#define SERVO_SAFE_MIDDLE 1000
#define SERVO_CLOSED 775

#define RED 0
#define GREEN 1
#define BLUE 2

float currentRGB[] = {0, 0, 0};
float changeRGB[] = {0, 0, 0};
byte newRGB[] = {0, 0, 0};

#define MODE_SLEEPING 0
#define MODE_BLOOM 1
#define MODE_BLOOMING 2
#define MODE_BLOOMED 3
#define MODE_FADE 4
#define MODE_FADING 5
#define MODE_FADED 6
#define MODE_FALLINGASLEEP 7

unsigned long time = 0;    //the last time the output pin was toggled

byte mode = MODE_SLEEPING;
byte modeChanged = MODE_SLEEPING;
byte brightness = 255;

byte petalPins[] = {5, 6, 7, 8, 10, 11};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(7, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ400);
Adafruit_TiCoServo servo;
 
int servoChange = 1; // open
int servoPosition = SERVO_SAFE_MIDDLE;

void setup() {
  Serial.begin(115200);
  pixels.begin();
  servo.attach(SERVO_PIN, SERVO_CLOSED, SERVO_OPEN);
  pinMode(TOUCH_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(TOUCH_SENSOR_PIN), _touchISR, RISING);

  randomSeed(analogRead(A7));
  SoftPWMBegin();

  pixelsUnifiedColor(pixels.Color(0, 0, 0));
  //pixelsUnifiedColor(pixels.Color(255, 70, 0));

  prepareCrossFade(140, 70, 0, 140);
  servo.write(servoPosition);
}

int counter = 0;
byte speed = 15;

void loop() {
  modeChanged = mode;
  boolean done = true;
  if (millis() - time > speed) {
    switch (mode) {
      case MODE_BLOOM:
        prepareCrossFadeBloom(500);
        changeMode(MODE_BLOOMING);
        break;
      case MODE_BLOOMING:
        done = crossFade() && done;
        done = openPetals() && done;
        done = petalsBloom(counter) && done;
        if (done) {
          changeMode(MODE_BLOOMED);
        }
        break;
      case MODE_FADE:
        //prepareCrossFade(0, 0, 0, 800);
        changeMode(MODE_FADING);
        break;
      case MODE_FADING:
        crossFade();
        closePetals();
        petalsFade(counter);
        break;
      case MODE_FADED:
        //prepareCrossFade(140, 70, 0, 140);
        changeMode(MODE_FALLINGASLEEP);
        break;
      case MODE_FALLINGASLEEP:
        done = crossFade() && done;
        done = closePetals() && done;
        pixelsUnifiedColor(0x000000);
        if (done) {
          changeMode(MODE_SLEEPING);
        }
        break;
    }
    if (modeChanged != mode) Serial.println(mode);
    time = millis();
    counter++;
  }
}

void changeMode(byte newMode) {
  if (mode != newMode) {
    mode = newMode;
    counter = 0;
  }
}

void _touchISR() {
  if (mode == MODE_SLEEPING) {
    changeMode(MODE_BLOOM);
  }
  else if (mode == MODE_BLOOMED) {
    changeMode(MODE_FADE);
  }
  else if (mode == MODE_FADING) {
    changeMode(MODE_FALLINGASLEEP);
  }
}

// petals animations

boolean petalsBloom(int j) {
  if (j < 250) {
    return false; // delay
  }
  if (j > 750) {
    return true;
  }
  int val = (j - 250) / 2;
  for (int i = 0; i < 6; i++) {
    SoftPWMSet(petalPins[i], val);
  }
  return false;
}

boolean petalsFade(int j) {
  if (j > 510) {
    return true;
  }
  for (int i = 0; i < 6; i++) {
    SoftPWMSet(petalPins[i], (510 - j) / 2);
  }
  return false;
}

// animations

void prepareCrossFadeBloom(unsigned int duration) {
  byte color = random(0, 5);
  switch (color) {
    case 0: // white
      prepareCrossFade(140, 140, 140, duration);
      break;
    case 1: // red
      prepareCrossFade(140, 5, 0, duration);
      break;
    case 2: // blue
      prepareCrossFade(30, 70, 170, duration);
      break;
    case 3: // pink
      prepareCrossFade(140, 0, 70, duration);
      break;
    case 4: // orange
      prepareCrossFade(255, 70, 0, duration);
      break;
  }
}

// servo function

boolean openPetals() {
  if (servoPosition >= SERVO_OPEN) {
    return true;
  }
  servoPosition ++;
  servo.write(servoPosition);
  return false;
}

boolean closePetals() {
  if (servoPosition <= SERVO_CLOSED) {
    return true;
  }
  servoPosition --;
  servo.write(servoPosition);
  return false;
}

// utility function

void pixelsUnifiedColor(uint32_t color) {
  for (unsigned int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}

void prepareCrossFade(byte red, byte green, byte blue, unsigned int duration) {
  float rchange = red - currentRGB[RED];
  float gchange = green - currentRGB[GREEN];
  float bchange = blue - currentRGB[BLUE];
  newRGB[RED] = red;
  newRGB[GREEN] = green;
  newRGB[BLUE] = blue;

  Serial.print(newRGB[RED]);
  Serial.print(" ");
  Serial.print(newRGB[GREEN]);
  Serial.print(" ");
  Serial.print(newRGB[BLUE]);
  Serial.print(" (");
  Serial.print(changeRGB[RED]);
  Serial.print(" ");
  Serial.print(changeRGB[GREEN]);
  Serial.print(" ");
  Serial.print(changeRGB[BLUE]);
  Serial.println(")");
}

boolean crossFade() {
  if (currentRGB[RED] == newRGB[RED] && currentRGB[GREEN] == newRGB[GREEN] && currentRGB[BLUE] == newRGB[BLUE]) {
    return true;
  }
  for (byte i = 0; i < 3; i++) {
    if (changeRGB[i] > 0 && currentRGB[i] < newRGB[i]) {
      currentRGB[i] = currentRGB[i] + changeRGB[i];
    }
    else if (changeRGB[i] < 0 && currentRGB[i] > newRGB[i]) {
      currentRGB[i] = currentRGB[i] + changeRGB[i];
    }
    else {
      currentRGB[i] = newRGB[i];
    }
  }
  pixelsUnifiedColor(pixels.Color(currentRGB[RED], currentRGB[GREEN], currentRGB[BLUE]));
  return false;
}

boolean ledsOff() {
  if (brightness == 0) 
    return true;
  else {
    brightness--;
    pixels.setBrightness(brightness);  
  }
  pixels.show();
  return false;
}

boolean ledsOn() {
  if (brightness == 255)
    return true;
  else {
    brightness++;
    pixels.setBrightness(brightness);
  }
  pixels.show();
  return false;
}

uint32_t colorWheel(byte wheelPos) {
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return pixels.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return pixels.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return pixels.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}
