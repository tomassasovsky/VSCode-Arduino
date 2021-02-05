#include <ESP32Servo.h>
#include <FastLED.h>
#include <IRremote.h>
#include "ArduinoJson.h" // To decode messages from Bluetooth
#include "ESPTrueRandom.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
//#define floower_debug

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// colors:
#define red 0xFDC03F
#define red1 0xFDC031
#define red2 0xFDC032
#define red3 0xFDC033
#define red4 0xFDC034
#define green 0xFDC035
#define green1 0xFDC036
#define green2 0xFDC037
#define green3 0xFDC038
#define green4 0xFDC039
#define blue 0xFDC03a
#define blue1 0xFDC03b
#define blue2 0xFDC03c
#define blue3 0xFDC03d
#define blue4 0xFDC03e
#define white 0xFDC05f

#define flash 0xFDC02b
#define stroke 0xFDC33c
#define fade 0xFDC04d
#define smooth 0xFD203e

#define colorMode 0
#define flashMode 1
#define strokeMode 2
#define fadeMode 3
#define smoothMode 4

#define redPin 25
#define greenPin 26
#define bluePin 27

#define RECV_PIN 34
IRrecv receiver(RECV_PIN);
decode_results results;

byte redValue = 0;
byte greenValue = 0;
byte blueValue = 0;
bool show = false;
byte mode = 0;

String bt = "";
DynamicJsonDocument doc(JSON_OBJECT_SIZE(6));

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "642804c8-b221-4219-a568-2e17b22490a2"
#define CHARACTERISTIC_UUID "81f16465-2a3e-4370-baf0-8489256d115f"

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
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if(value.length() > 0) {
      Serial.println(value.c_str());
      bt = value.c_str();
    }
  }
};

void MainCoreCode(void *pvParameters) {
  while(1) {
    readReceiver();
    if (bt != "") {
      deserializeJson(doc, bt);
      if (doc["changeMode"] == 1) {
        #ifdef floower_debug
        Serial.println("changing mode");
        #endif
        show = !show;
      }
      redValue = (int)doc["red"];
      greenValue = (int)doc["green"];
      blueValue = (int)doc["blue"];
      updateLEDs();
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
    delay(1);
  }
}

void BluetoothCoreCode(void *pvParameters) {
  BLEDevice::init("RGB LED Strip");
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
  receiver.enableIRIn();
  delay(500);
  xTaskCreatePinnedToCore(BluetoothCoreCode, "BluetoothCore", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MainCoreCode, "MainCoreTask", 10000, NULL, 1, NULL, 0);
}

void loop() {}

void readReceiver() {
  if (receiver.decode()) {
    if (receiver.results.value != 0xFFFFFFFF) {
      switch (receiver.results.value) {
        case red:
          setColor(255, 0, 0);
          break;
        case red1:
          setColor(208, 56, 17);
          break;
        case red2:
          setColor(215, 92, 24);
          break;
        case red3:
          setColor(216, 118, 19);
          break;
        case red4:
          setColor(240, 240, 64);
          break;
        case green:
          setColor(0, 255, 0);
          break;
        case green1:
          setColor(8, 142, 73);
          break;
        case green2:
          setColor(16, 114, 203);
          break;
        case green3:
          setColor(22, 78, 173);
          break;
        case green4:
          setColor(44, 40, 114);
          break;
        case blue:
          setColor(0, 0, 255);
          break;
        case blue1:
          setColor(29, 37, 148);
          break;
        case blue2:
          setColor(35, 23, 63);
          break;
        case blue3:
          setColor(86, 56, 66);
          break;
        case blue4:
          setColor(220, 59, 135);
          break;
        case white:
          setColor(255, 255, 255);
          break;
        case flash:
          changeMode(flashMode);
          break;
        case stroke:
          changeMode(strokeMode);
          break;
        case fade:
          changeMode(fadeMode);
          break;
        case smooth:
          changeMode(smoothMode);
          break;
      }
    }
    receiver.resume();
  }
}

void changeMode(byte newMode) {

}

void setColor(byte r, byte g, byte b) {
  show = true;
  redValue = r;
  greenValue = g;
  blueValue = b;
  updateLEDs();
}

void updateLEDs() {
  if (show) {
    analogWrite(redPin, redValue);
    analogWrite(greenPin, greenValue);
    analogWrite(bluePin, blueValue);
  } else {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
  }
}