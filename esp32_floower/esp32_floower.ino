#include <SoftwareSerial.h>

SoftwareSerial BT(3, 4); 
String bt = "";

void setup() {
  BT.begin(9600);
  Serial.begin(9600);
}

void loop() {
  if(BT.available()){
    bt = BT.readString();
  }
  Serial.println(bt);
  while(!BT.available());
}