#include <IRremote.h>

#define RECV_PIN 34
IRrecv receiver(RECV_PIN);
decode_results results;

void setup(){
  Serial.begin(115200);
  receiver.enableIRIn();
}

void loop(){
  if (receiver.decode()) {
    if (receiver.results.value != 0xFFFFFFFF) {
      Serial.println(receiver.results.value, HEX);
    }
    receiver.resume();
  }
}