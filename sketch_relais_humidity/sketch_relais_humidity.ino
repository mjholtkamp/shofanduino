#include <Time.h>

const int relaisPin =  11;
const int sensorPin = A0;    // select the input pin for the potentiometer
const int MAXVALUE = 1023;
int sensorValue = 0;  // variable to store the value coming from the sensor
int thresholdLow = 0;
int thresholdHigh = 0;
int dayMax = 0;
int dayMin = 0;
int firstRun = 1;
int prevday = 0;
int diff;
int avg;
int curday;

void setup() {
  // initialize the 11 pin as an output:
  Serial.begin(9600);
  pinMode(relaisPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(relaisPin, HIGH);
}

void loop(){
  sensorValue = analogRead(sensorPin);
  sensorValue = MAXVALUE - sensorValue;
  curday = day();
  if (firstRun == 1) {
    // first run, initialize threshold to sane values
    firstRun = 0;
    prevday = curday;
    thresholdLow = constrain(sensorValue - 10, 0, MAXVALUE);
    thresholdHigh = constrain(sensorValue + 10, 0, MAXVALUE);
    dayMin = thresholdLow;
    dayMax = thresholdHigh;
  }
  dayMin = constrain(sensorValue, 0, dayMin);
  dayMax = constrain(sensorValue, dayMax, MAXVALUE);
  if (prevday != curday) {
    prevday = curday;
    diff = dayMax - dayMin;
    avg = dayMin + diff / 2;
    thresholdLow = avg - diff / 4;
    thresholdHigh = avg + diff / 4;
    dayMin = sensorValue;
    dayMax = sensorValue;
  }

  Serial.print("humidity = ");
  Serial.print(sensorValue);
  Serial.print(", tlow = ");
  Serial.print(thresholdLow);
  Serial.print(", thi = ");
  Serial.print(thresholdHigh);
  Serial.print(", dmin = ");
  Serial.print(dayMin);
  Serial.print(", dmax = ");
  Serial.println(dayMax);

  if (sensorValue < thresholdLow) {
    // HIGH means setting the relay to NC
    digitalWrite(relaisPin, HIGH);
  } else if (sensorValue > thresholdHigh) {
    // LOW means setting the relay to NO
    digitalWrite(relaisPin, LOW); 
  }
  delay(1000);
}
