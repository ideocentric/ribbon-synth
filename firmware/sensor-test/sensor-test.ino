#include "DaisyDuino.h"

const int kAdcBits = 12;
const long kAdcMax = 4095;

DaisyHardware hw;
size_t numChannels;


const int kBlackPin = A7;  
const int kWhitePin = A0;
const int kGrayPin = A6;
const int kSw1Pin = D13;
const int kSw2Pin = D14;
const int kOrangePin = A3;
const int kYellowPin = A4;
const int kGreenPin = A2;
const int kBluePin = A5;
const int kPurplePin = A1;
const int kForceSensorPin = A8;
const int kSoftPotPin = A9;

void setup() {
  analogReadResolution(kAdcBits);
  pinMode(kSw1Pin, INPUT);
  pinMode(kSw2Pin, INPUT);
  Serial.begin(9600);
}

unsigned long lastPrintTime = 0;
unsigned long now = millis();

void loop() {
  long blackReading, whiteReading, grayReading, sw1Reading, sw2Reading, orangeReading, yellowReading, greenReading, blueReading, purpleReading, forceSensorReading, softPotReading;
  float black, white, gray, sw1, sw2, orange, yellow, green, blue, purple, forceSensor, softPot;
  
  blackReading = analogRead(kBlackPin);
  black = blackReading / (float) kAdcMax;

  whiteReading = analogRead(kWhitePin);
  white = whiteReading / (float) kAdcMax;

  grayReading = analogRead(kGrayPin);
  gray = grayReading / (float) kAdcMax;

  sw1Reading = digitalRead(kSw1Pin);
  if(sw1Reading > 0)
  {
    sw1 = 1.0;
  }
  else
  {
    sw1 = 0.0;
  }

  sw2Reading = digitalRead(kSw2Pin);
  if(sw2Reading > 0)
  {
    sw2 = 1.0;
  }
  else
  {
    sw2 = 0.0;
  }

  orangeReading = analogRead(kOrangePin);
  orange = orangeReading / (float) kAdcMax;

  yellowReading = analogRead(kYellowPin);
  yellow = yellowReading / (float) kAdcMax;

  greenReading = analogRead(kGreenPin);
  green = greenReading / (float) kAdcMax;

  blueReading = analogRead(kBluePin);
  blue = blueReading / (float) kAdcMax;

  purpleReading = analogRead(kPurplePin);
  purple = purpleReading / (float) kAdcMax;

  forceSensorReading = analogRead(kForceSensorPin);
  forceSensor = forceSensorReading / (float) kAdcMax;

  softPotReading = analogRead(kSoftPotPin);
  softPot = softPotReading / (float) kAdcMax;

  now = millis();
  if((now - lastPrintTime > 100))
  {
    lastPrintTime = now;    
    Serial.print("gray:");
    Serial.print(gray);
    Serial.print(",");
    Serial.print("white:");
    Serial.print(white);
    Serial.print(",");
    Serial.print("black:");
    Serial.print(black);
    Serial.print(",");
    Serial.print("sw1:");
    Serial.print(sw1);
    Serial.print(",");
    Serial.print("sw2:");
    Serial.print(sw2);
    Serial.print(",");
    Serial.print("purple:");
    Serial.print(purple);
    Serial.print(",");
    Serial.print("blue:");
    Serial.print(blue);
    Serial.print(",");
    Serial.print("green:");
    Serial.print(green);
    Serial.print(",");
    Serial.print("yellow:");
    Serial.print(yellow);
    Serial.print(",");
    Serial.print("orange:");
    Serial.print(orange);
    Serial.print(",");
    Serial.print("forceSensor:");
    Serial.print(forceSensor);
    Serial.print(",");
    Serial.print("softPot:");
    Serial.println(softPot);
  }

}
