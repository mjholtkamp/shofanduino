#include <Time.h>

const int relaisPin =  11;
const int sensorPin = A0;    // select the input pin for the potentiometer
const int MINHUMIDITY = 0;
const int MAXHUMIDITY = 1023;
const int milliseconds_sleep = 5000;
const int avg_decay = 0.00001;
const int deviation_decay = 0.01;
int sensorValue = 0;  // variable to store the value coming from the sensor
int thresholdLow = 0;
int thresholdHigh = 0;
int avg = 0;
int deviation = 20;
int humidity = 0;
int startup_seconds = 900; // sensor needs to settle for 15 minutes

int get_humidity() {
  sensorValue = analogRead(sensorPin);
  
  // sensor value is high when humidity is low, this is
  // counterintuitive (it's a dryness sensor!), so we
  // reverse it:
  return MAXHUMIDITY - sensorValue;
}

void setup() {
  // initialize the 11 pin as an output:
  Serial.begin(9600);
  pinMode(relaisPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(relaisPin, HIGH);
  avg = get_humidity();
}

void loop() {
  humidity = get_humidity();
  
  if (startup_seconds > 0) {
    // during startup, sensor needs to settle, so average is allowed to change fast
    avg = (avg + humidity) / 2;
    startup_seconds -= milliseconds_sleep / 1000;
  } else {
    // decay the old value, combine with the new (reverse decay)
    avg = avg * (1.0 - avg_decay) + humidity * avg_decay;
  }
  int cur_deviation = max(deviation, humidity - avg);
  deviation = deviation * (1.0 - deviation_decay) + cur_deviation * deviation_decay;
  thresholdLow = avg + deviation / 3;
  thresholdHigh = avg + 2 * deviation / 3;

  // print out the values for debugging/plotting
  Serial.print("humidity = ");
  Serial.print(humidity);
  Serial.print(", tlow = ");
  Serial.print(thresholdLow);
  Serial.print(", thi = ");
  Serial.print(thresholdHigh);
  Serial.print(", dmin = ");
  Serial.print(avg);
  Serial.print(", dmax = ");
  Serial.println(avg + deviation);

  // switch the relays!
  if (sensorValue < thresholdLow) {
    // HIGH means setting the relay to NC
    digitalWrite(relaisPin, HIGH);
  } else if (sensorValue > thresholdHigh) {
    // LOW means setting the relay to NO
    digitalWrite(relaisPin, LOW); 
  }
  delay(milliseconds_sleep);
}
