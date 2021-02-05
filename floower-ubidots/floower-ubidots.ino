#include "UbidotsESPMQTT.h"
#include <FastLED.h>

#define DEVICE "floower"  // Put here your Ubidots device label
#define TOKEN "BBFF-Y5yh3G3nM6UaVVBg00iAIwvneC13y1"  // Put here your Ubidots TOKEN

#define WIFISSID "MyHouseWTF 2.4GHz" // Put here your Wi-Fi SSID
#define PASSWORD "shaidallar" // Put here your Wi-Fi password

#define pinLEDs 5 
#define qtyLEDs 1
#define typeOfLEDs WS2812B    //type of LEDs i'm using
CRGB LEDs[qtyLEDs];

#define stateVar State
byte red = 0;
byte green = 150;
byte blue = 136;

Ubidots client(TOKEN);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200); 
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LED
  client.wifiConnection(WIFISSID, PASSWORD);
  if(!client.connected()){
    client.reconnect();
  }
  client.begin(callback);
  client.add("State", 0);
  client.add("red", red);
  client.add("green", green);
  client.add("blue", blue);
  client.ubidotsPublish(DEVICE);
}

void loop() {
  client.loop();
  fill_solid(LEDs, qtyLEDs, CRGB(red, green, blue));
  FastLED.show();
}

void execute_cases(int variable) {
  switch (variable) {
    case 0:
      Serial.println("Luz: ");
      break;
    case 1:
      Serial.println("Ventilador: ");
      break;
    case 65535: //error value
      Serial.println("error");
      break;
  }
}