#include "ArduinoJson.h" // To decode messages from Bluetooth
#include <ESP32Servo.h>
#include <FastLED.h>
#include "ESPTrueRandom.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
//#define floower_debug

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define pinLEDs 26
#define qtyLEDs 1
#define typeOfLEDs WS2812B   //type of LEDs i'm using
CRGB LEDs[qtyLEDs];

#define servoPin 25
#define servoOpen 45
#define servoClosed 135

int servoPosition = servoClosed;
int newServoPosition = servoClosed;

byte red = 0;
byte green = 0;
byte blue = 0;
byte brightness = 0;
byte newBrightness = 0;

#define buttonPin 32
byte mode = 0;
bool done = true;
bool changed = false;

unsigned long LEDsTime = 0;
unsigned long servoTime = 0;
#define LEDsDelay 20
#define servoDelay 100

#define shortPress 500

bool lastState = LOW;  // the previous state from the input pin
bool currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

Servo servo;

String bt = "";
DynamicJsonDocument doc(JSON_OBJECT_SIZE(6));

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string value = pCharacteristic->getValue();

    if(value.length() > 0){
      Serial.println(value.c_str());
      bt = value.c_str();
    }
  }
};

void MainCoreCode(void *pvParameters) {
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LEDs
  FastLED.setBrightness(brightness);
  FastLED.show();
  servo.attach(servoPin);
  while(1) {
    if (bt != "") {
      deserializeJson(doc, bt);
      if (doc["changeMode"] == 1 && done) {
        #ifdef floower_debug
        Serial.println("changing mode");
        #endif
        changeMode();
      }
      red = (int)doc["red"];
      green = (int)doc["green"];
      blue = (int)doc["blue"];
      #ifdef floower_debug
      Serial.print("change mode: ");
      serializeJson(doc["changeMode"], Serial);
      Serial.println();
      Serial.print("red: ");
      serializeJson(doc["red"], Serial);
      Serial.println();
      Serial.print("green: ");
      serializeJson(doc["green"], Serial);
      Serial.println();
      Serial.print("blue: ");
      serializeJson(doc["blue"], Serial);
      Serial.println();
      #endif
      bt = "";
    }
    currentState = digitalRead(buttonPin);
    if(lastState == LOW && currentState == HIGH) {
      pressedTime = millis();
    } else if(lastState == HIGH && currentState == LOW) { // button is released
      releasedTime = millis();
      long pressDuration = releasedTime - pressedTime;

      if (pressDuration < shortPress) {
        changeModeTouch();
      } else {
        generateNewColor();
        changeModeTouch();
      }
    }
    lastState = currentState;
    updateLED();
    update();
    delay(1);
  }
}

void BluetoothCoreCode(void *pvParameters) {
  BLEDevice::init("ESP32 GET NOTI FROM DEVICE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  #ifdef floower_debug
  Serial.println("The device started, now you can pair it with bluetooth!");
  #endif
  while(1) {
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    delay(1);
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(21));
  pinMode(32, INPUT_PULLUP);
  delay(500);
  xTaskCreatePinnedToCore(BluetoothCoreCode, "BluetoothCore", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MainCoreCode, "MainCoreTask", 10000, NULL, 1, NULL, 0);
}

void loop() {}

void updateLED() {
  LEDs[0] = CRGB(red, green, blue);
  if (mode != 0) {
    FastLED.show();
  }
}

void changeMode() {
  if (mode != 2) {
    mode++;
  } else
    mode = 0;
}

void changeModeTouch() {
  if (done) {
    printf("changing\n");
    changeMode();
    changed = true;
  }
}

void generateNewColor() {
  if (brightness == 0 && done) {
    red = random(0, 255);
    green = random(0, 255);
    blue = random(0, 255);
  }
}

void update() {
  done = !changed;
  done = (servoPosition == newServoPosition) && done;
  switch (mode) {
    case 0:
      newBrightness = 0;
      break;
    case 1:
      newBrightness = 255;
      newServoPosition = servoOpen;
      break;
    case 2:
      newServoPosition = servoClosed;
      
      break;
  }
  if (millis() - LEDsTime > LEDsDelay) {
    updateBrightness();
    LEDsTime = millis();
  }
  if (millis() - servoTime > servoDelay) {
    updateServo();
    servoTime = millis();
  }
  servo.write(servoPosition);
}

void updateBrightness() {
  if (brightness != newBrightness) {
    if (brightness > newBrightness) brightness--;
    else brightness++;
    fill_solid(LEDs, qtyLEDs, CRGB(red, green, blue));
    FastLED.setBrightness(brightness);
    FastLED.show();
    changed = true;
  } else {
    changed = false;
  }
}

void updateServo() {
  if (servoPosition > newServoPosition)
    servoPosition--, done = false;
  else if (servoPosition < newServoPosition)
    servoPosition++, done = false;
}