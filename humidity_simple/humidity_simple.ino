#include <Time.h>

const int relayPin =  11;
const int sensorPin = A0;    // select the input pin for the potentiometer
const int MINHUMIDITY = 0;
const int MAXHUMIDITY = 1023;
const int milliseconds_sleep = 5000;
const int avg_decay = 0.00001;
const int deviation_decay = 0.01;
int thresholdLow = 0;
int thresholdHigh = 0;
int avg = 0;
int deviation = 20;
int humidity = 0;
int startup_seconds = 900; // sensor needs to settle for 15 minutes

int get_humidity() {
  int sensorValue = analogRead(sensorPin);
  
  // sensor value is high when humidity is low, this is
  // counterintuitive (it's a dryness sensor!), so we
  // reverse it:
  return MAXHUMIDITY - sensorValue;
}

void setup() {
  // initialize the 11 pin as an output:
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(relayPin, HIGH);
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
  // don't use math inside max(), it causes undefined behaviour (see docs)
  int cur_deviation = humidity - avg;
  int max_deviation = max(deviation, cur_deviation);
  deviation = deviation * (1.0 - deviation_decay) + max_deviation * deviation_decay;
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
  if (humidity < thresholdLow) {
    // HIGH means setting the relay to NC
    digitalWrite(relayPin, HIGH);
  } else if (humidity > thresholdHigh) {
    // LOW means setting the relay to NO
    digitalWrite(relayPin, LOW); 
  }
  delay(milliseconds_sleep);
}
