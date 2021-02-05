#define BLYNK_PRINT Serial
#define FASTLED_HAS_CLOCKLESS
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include <EEPROM.h>

//#define _DEBUG

#define pinLEDs 0
#define servoPin 4

#define qtyLEDs 1
#define typeOfLEDs WS2812B   //type of LEDs i'm using
CRGB LEDs[qtyLEDs];


#define servoOpen 45
#define servoClosed 135

int servoPosition = servoClosed;
int newServoPosition = servoClosed;

byte red = 0;
byte green = 0;
byte blue = 0;
byte brightness = 0;
byte newBrightness = 0;

byte mode = 0;
int buttonpress = 0;
int lastbuttonpress = 0;
bool done = true;
bool changed = false;
bool isConnected = false;
bool blynkIsConfigured = false;
bool LEDState = true;

unsigned long LEDsTime = 0;
unsigned long servoTime = 0;
#define LEDsDelay 20
#define servoDelay 100

IPAddress local_ip(8,8,8,8);
IPAddress gateway(8,8,8,1);
IPAddress netmask(255,255,255,0);

String webConnect = "<html> <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/> <title>WiFi - ESP8266</title> <style type=\"text/css\"> body,td,th { color: rgb(255, 255, 255); } </style> </head> <body style=background-color:#3d3d3d;> <form action=\"config\" method=\"get\" target=\"pantalla\"> <fieldset style=\"border-style:solid; border-color:#ffffff; width:160px; height:200px; padding:10px; margin: 5px;\"> <legend><strong>Configure WiFi</strong></legend> WiFi SSID <br/> <input name=\"ssid\" id\"ssid\" type=\"text\" size=\"15\"/> <br/><br/> Password <br/> <input name=\"pass\" type=\"password\" size=\"15\" id=\"pass\"/> <br/><br/> <input type=\"checkbox\" onclick=\"myFunction()\"> Show Password <br/><br/> <input type=\"submit\" value=\"Connect\" /> </fieldset> </form> <iframe id=\"pantalla\" name=\"pantalla\" src=\"\" width=900px height=400px frameborder=\"0\" scrolling=\"no\"></iframe> <script> function myFunction() { var x = document.getElementById('pass'); if (x.type === \"password\") { x.type = \"text\"; } else { x.type = \"password\"; } } </script> </body> </html>";

ESP8266WebServer server(80);

char ssid[20];
char pass[20];
char auth[] = "BQ37g6guB-XJIaaKuSRfqk4CcWTGO4uV";

String ssidRead;
String passRead;
byte ssidSize = 0;
byte passSize = 0;

const char *APSSID = "Floower";
Servo servo;

BLYNK_WRITE(V0) {
  red = param.asInt();
  updateLED();
} 

BLYNK_WRITE(V1) {
  green = param.asInt();
  updateLED();
}

BLYNK_WRITE(V2) {
  blue = param.asInt();
  updateLED();
}

BLYNK_WRITE(V3) {
  if (done)
    changeMode();
}

BLYNK_READ(V4) {
  Blynk.virtualWrite(V5, mode);
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(4096);
  pinMode(5, INPUT_PULLUP);
  pinMode(2, OUTPUT);
  delay(2000);
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LEDs
  FastLED.setBrightness(brightness);
  FastLED.show();
  initAP();
  if (digitalRead(5) == HIGH) {
    connectWifi();
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected succesfully to " + String(ssid) + ".");
    digitalWrite(2, LOW);
  }
  servo.attach(4);
}

void loop() {
  server.handleClient();
  if (WiFi.status() == WL_CONNECTED) {
    if (!blynkIsConfigured) {
      Blynk.config(auth, IPAddress(192,168,0,90), 8080);
      blynkIsConfigured = true;
    }
    Blynk.run();
    update();
  }
}

void updateLED() {
  LEDs[0] = CRGB(red, green, blue);
  FastLED.show();
}

void changeMode() {
  if (mode != 2)
    mode++;
  else
    mode = 0;
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

void connectWifi() {
  if (wifiRead(70).equals("configured")) {
    ssidRead = wifiRead(1);      //wifiReadmos ssid y password
    passRead = wifiRead(30);

    ssidSize = ssidRead.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
    passSize = passRead.length() + 1;

    ssidRead.toCharArray(ssid, ssidSize); //Transf. el String en un char array ya que es lo que nos pide WiFi.begin()
    passRead.toCharArray(pass, passSize);
  
    byte i = 0;
    WiFi.begin(ssid, pass);      //Intentamos conectar
    while (WiFi.status() != WL_CONNECTED) {
      LEDState = !LEDState;
      digitalWrite(2, LEDState);
      delay(1000);
      Serial.println("Connecting...");
      LEDState = !LEDState;
      digitalWrite(2, LEDState);
      delay(1000);
      if (i == 9){
        Serial.println("Connecting to the WiFi " + String(ssid) + " was not possible.");
        digitalWrite(2, HIGH);
        return;
      } else i++;
    }
    digitalWrite(2, LOW);
  }
}
void initAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, netmask);
  WiFi.softAP(APSSID);
  server.on("/", []() {
    server.send(200, "text/html", webConnect);
  });
  server.on("/config", wifiConfig);
  server.begin();
  Serial.println("Access Point " + String(APSSID) + " initialized.");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println(IP);
  #ifdef _DEBUG
    Serial.println(wifiRead(70));
    Serial.print("Saved SSID: ");
    Serial.println(wifiRead(1));
    Serial.print("Saved Password: ");
    Serial.println(wifiRead(30));
  #endif
}
void wifiConfig() {
  digitalWrite(2, LOW);
  String getssid = server.arg("ssid"); //Recibimos los valores que envia por GET el formulario web
  String getpass = server.arg("pass");

  getssid = symbolFixer(getssid); //Reemplazamos los simbolos que aparecen cun UTF8 por el simbolo correcto
  getpass = symbolFixer(getpass);

  ssidSize = getssid.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
  passSize = getpass.length() + 1;

  getssid.toCharArray(ssid, ssidSize); //Transformamos el string en un char array ya que es lo que nos pide WIFI.begin()
  getpass.toCharArray(pass, passSize);

  WiFi.begin(ssid, pass);     //Intentamos conectar
  delay(1000);
  byte i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    LEDState = !LEDState;
    digitalWrite(2, LEDState);
    delay(1000);
    Serial.println("Connecting...");
    LEDState = !LEDState;
    digitalWrite(2, LEDState);
    delay(1000);
    if (i == 9) {
      Serial.println("Connection failed!");
      digitalWrite(2, HIGH);
      saveEEPROM(70, "notconfigured");
      Serial.println("WiFi SSID or Password are incorrect. The data hasn't been saved.");
      server.send(200, "text/html", String("<style type=\"text/css\"> body,td,th { color: rgb(255, 255, 255); } </style> <h2 style=\"font-size:14px;\"> WiFi SSID or Password <br/> are incorrect.<br/>The data hasn't <br/> been saved.</h2>"));
      return;
    } else i++;
  }
  #ifdef _DEBUG
  Serial.println(WiFi.localIP());
  #endif
  saveEEPROM(70, "configured");
  saveEEPROM(1, getssid);
  saveEEPROM(30, getpass);
  digitalWrite(2, LOW);
  Serial.println("Succesful connection to: " + getssid + ". Data has been saved.");
  server.send(200, "text/html", String("<style type=\"text/css\"> body,td,th { color: rgb(255, 255, 255); } </style> <h2 style=\"font-size:14px;\">Succesful connection to: <br/>" + getssid + "<br/><br/>Data has been saved."));
}
void saveEEPROM(int addr, String a) {
  int size = (a.length() + 1);
  char inchar[size];
  a.toCharArray(inchar, size);
  EEPROM.write(addr, size);
  for (int i = 0; i < size; i++) {
    addr++;
    EEPROM.write(addr, inchar[i]);
  }
  EEPROM.commit();
}
String wifiRead(int addr) {
  String newString;
  int value;
  int size = EEPROM.read(addr);
  for (int i = 0; i < size; i++) {
    addr++;
    value = EEPROM.read(addr);
    newString += (char)value;
  }
  return newString;
}
String symbolFixer(String a) {
  a.replace("%C3%A1", "á");
  a.replace("%C3%A9", "é");
  a.replace("%C3%A", "i");
  a.replace("%C3%B3", "ó");
  a.replace("%C3%BA", "ú");
  a.replace("%21", "!");
  a.replace("%23", "#");
  a.replace("%24", "$");
  a.replace("%25", "%");
  a.replace("%26", "&");
  a.replace("%27", "/");
  a.replace("%28", "(");
  a.replace("%29", ")");
  a.replace("%3D", "=");
  a.replace("%3F", "?");
  a.replace("%27", "'");
  a.replace("%C2%BF", "¿");
  a.replace("%C2%A1", "¡");
  a.replace("%C3%B1", "ñ");
  a.replace("%C3%91", "Ñ");
  a.replace("+", " ");
  a.replace("%2B", "+");
  a.replace("%22", "\"");
  return a;
}