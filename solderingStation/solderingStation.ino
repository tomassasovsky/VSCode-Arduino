#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

#define tempSensor A0
#define knob A7
#define IRON_PIN 10
#define LED_PIN 8

CRGB LED[1];

int
minTemp = 27,       //Minimum aquired iron tip temp during testing (°C)
maxTemp = 525,      //Maximum aquired iron tip temp during testing (°C)
minADC = 234,      //Minimum aquired ADC value during minTemp testing
maxADC = 733,      //Maximum aquired ADC value during minTemp testing

maxPWM = 255,    //Maximum PWM Power
avgCounts = 5,      //Number of avg samples
lcdInterval = 80,   //LCD refresh rate (miliseconds) 

pwm = 0,
tempRAW = 0,
knobRAW = 0,
counter = 0,
setTemp = 0,
setTempAVG = 0,
currentTempAVG = 0,
previousMillis = 0;

float 
currentTemp = 0.0,
store = 0.0,
knobStore = 0.0;

byte tempCharacter[] = {
	0b00100,
	0b01010,
	0b01010,
	0b01010,
	0b10001,
	0b10001,
	0b01110,
	0b00000
};

void setup(){
	pinMode(tempSensor, INPUT);			//Set Temp Sensor pin as INPUT
	pinMode(knob, INPUT);						//Set Potentiometer Knob as INPUT
	pinMode(IRON_PIN, OUTPUT);			//Set MOSFET PWM pin as OUTPUT
	pinMode(A6, INPUT);							//Passthru Pin
	FastLED.addLeds<WS2812B, LED_PIN>(LED, 1);
	lcd.init();
	lcd.backlight();
	lcd.clear();
  lcd.createChar(0, tempCharacter);
	lcd.setCursor(0,1);
	lcd.write((byte)0);
	lcd.print("  ");
	lcd.setCursor(0,0);
	lcd.print("ACTUAL T:");
}

void loop(){
	//--------Gather Sensor Data--------//
	knobRAW = analogRead(knob); //Get analog value of Potentiometer
	setTemp = map(knobRAW, 0, 1023, minTemp, maxTemp);  //Scale pot analog value into temp unit

	tempRAW = analogRead(tempSensor);  //Get analog value of temp sensor
	currentTemp = map(tempRAW, minADC, maxADC, minTemp, maxTemp);  //Sacle raw analog temp values as actual temp units
	
	//--------Get Average of Temp Sensor and Knob--------//
	if(counter < avgCounts) {  //Sum up temp and knob data samples
		store = store + currentTemp;
		knobStore = knobStore + setTemp;
		++counter;
	} else {
		currentTempAVG = (store/avgCounts) -1;  //Get temp mean (average)
		setTempAVG = (knobStore/avgCounts);  //Get knob - set temp mean (average)
		knobStore = 0;  //Reset storage variable
		store = 0;      //Reset storage variable
		counter = 0;    //Reset storage variable
	}
	
	//--------PWM Soldering Iron Power Control--------//
	if(analogRead(knob) == 0) {  //Turn off iron when knob as at its lowest (iron shutdown)
		LED[0] = CRGB::Green;
		pwm = 0;
	}
	else if(currentTemp <= setTemp) {  //Turn on iron when iron temp is lower than preset temp
		LED[0] = CRGB::Red;
		pwm = maxPWM;
	} else {  //Turn off iron when iron temp is higher than preset temp
		LED[0] = CRGB::Green;
		pwm = 0;
	}
	FastLED.show();
	analogWrite(IRON_PIN, pwm);  //Apply the aquired PWM value from the three cases above

	//--------Display Data--------//
	unsigned long currentMillis = millis(); //Use and aquire millis function instead of using delay
	if (currentMillis - previousMillis >= lcdInterval){ //LCD will only display new data ever n milisec intervals
		previousMillis = currentMillis;
 
		if(analogRead(knob) == 0) {
			lcd.setCursor(10, 1);
			lcd.print("OFF  ");
		}
		else {
			lcd.setCursor(10, 1);
			lcd.print(setTempAVG, 1);
			lcd.print((char)223);
			lcd.print("C ");
		}
		
		if(currentTemp<minTemp) {
			lcd.setCursor(10, 0);
			lcd.print("COOL ");
		}
		else{
			lcd.setCursor(10, 0);
			lcd.print(currentTempAVG, 1);
			lcd.print((char)223);
			lcd.print("C ");
		}   
	}
}