#include <Time.h>

// consts for controlling relays
const int K1 = 11;
const int K2 = 12;
const int milliseconds_switch_delay = 100;
const int NormallyConnected = HIGH;
const int NormallyOpen = LOW;


// consts/variables for humidity sensor and averages etc.
const int sensorPin = A0;    // select the input pin for the potentiometer
const int MINHUMIDITY = 0;
const int MAXHUMIDITY = 1023;
const int milliseconds_sleep = 5000;
const float avg_decay = 0.00001;
const float deviation_decay = 0.01;

int thresholdLow = 0;
int thresholdHigh = 0;
float avg = 0;
float deviation = 40;
int humidity = 0;
int startup_seconds = 900; // sensor needs to settle for 15 minutes
int on_off = 0;

int get_humidity() {
  int sensorValue = analogRead(sensorPin);
  
  // sensor value is high when humidity is low, this is
  // counterintuitive (it's a dryness sensor!), so we
  // reverse it:
  return MAXHUMIDITY - sensorValue;
}

void setup() {
  Serial.begin(9600);
  
  // set relays to normally connected
  pinMode(K1, OUTPUT);
  pinMode(K2, OUTPUT);
  digitalWrite(K1, NormallyConnected);
  digitalWrite(K2, NormallyConnected);

  // initialize sensor
  pinMode(sensorPin, INPUT);
  avg = get_humidity();
}

void loop() {
  humidity = get_humidity();
  
  if (startup_seconds > 0) {
    // during startup, sensor needs to settle, so average is allowed to change fast
    avg = (avg + humidity) / 2.0f;
    startup_seconds -= milliseconds_sleep / 1000;
  } else {
    // decay the old value, combine with the new (reverse decay)
    avg = avg * (1.0f - avg_decay) + humidity * avg_decay;
  }
  // don't use math inside max(), it causes undefined behaviour (see docs)
  int cur_deviation = humidity - avg;
  int max_deviation = max(deviation, cur_deviation);
  deviation = deviation * (1.0f - deviation_decay) + max_deviation * deviation_decay;
  thresholdLow = avg + deviation / 3.0f;
  thresholdHigh = avg + 2.0f * deviation / 3.0f;

  on_off = avg;
  // switch the relays!
  if (humidity < thresholdLow) {
    // because we don't know the status of the 3-way switch, we switch both
    // relays at the same time and hope there is no short circuit. This should
    // not be an issue, but anyway...
    digitalWrite(K1, NormallyConnected);
    digitalWrite(K2, NormallyConnected);
    on_off -= 20;
  } else if (humidity > thresholdHigh) {
    digitalWrite(K2, NormallyOpen);
    // since we don't know how the switch is switched, we first switch K2 to
    // non-connected, then wait a while for it to settle, before we switch
    // K1 to connected L. This should prevent any shorts (don't know if that
    // is actually an issue, but better safe than sorry).
    delay(milliseconds_switch_delay);
    digitalWrite(K1, NormallyOpen);
    on_off -= 10;
  } else {
    on_off -= 15;
  }
   // print out the values for debugging/plotting
  Serial.print("humidity = ");
  Serial.print(humidity);
  Serial.print(", tlow = ");
  Serial.print(thresholdLow);
  Serial.print(", thi = ");
  Serial.print(thresholdHigh);
  Serial.print(", avg = ");
  Serial.print((int)(avg));
  Serial.print(", deviation = ");
  Serial.print((int)(avg + deviation));
  Serial.print(", on_off = ");
  Serial.println((int)(on_off));
  
  delay(milliseconds_sleep);
}
