#include <IRremote.h>
IRsend irsend;

#define button1 4
#define button2 5
#define button3 6

void setup() {
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
}

void loop() {
  if (digitalRead(button1) == HIGH){
    timer = 0;  
    delay(50);
    irsend.sendNEC(0x34895725, 32);
  } //Vol+

  if (digitalRead(button2) == HIGH){
    timer = 0;
    delay(50);
    irsend.sendNEC(0x56874159, 32);
  } //Switch

  if (digitalRead(button3) == HIGH){
    timer = 0;
    delay(50);
    irsend.sendNEC(0x15467823, 32);
  } //Vol-
  
  delay(1);
  timer = timer + 1;
}