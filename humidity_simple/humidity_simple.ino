#include <Time.h>

// consts for controlling relays
const int K1 = 11;
const int K2 = 12;
const int milliseconds_switch_delay = 100;
const int NormallyConnected = HIGH;
const int NormallyOpen = LOW;
enum {FAN_OFF, FAN_NOCHANGE, FAN_ON};


// consts/variables for humidity sensor and averages etc.
const int sensorPin = A0;    // select the input pin for the potentiometer
const int MINHUMIDITY = 0;
const int MAXHUMIDITY = 1023;
const int milliseconds_sleep = 5000;
const float avg_ratio = 0.00002; // ratio of new value to old value (1.0 = new value)
const float deviation_ratio = 0.000001;

int thresholdLow = 0;
int thresholdHigh = 0;
float avg = 0;
float deviation = 60;
int humidity = 0;
int startup_seconds = 900; // sensor needs to settle for 15 minutes
int on_off = 0;

// variables to decide the state of the fan
int action = FAN_OFF; // action to take, based on sensor reading
int cur_state = FAN_OFF; // the actual state of the fan
int new_state = FAN_OFF; // new state that fan would be in, if state_count is high enough
int state_count = 0;   // only change the state if the count is 3 or higher, to prevent sensor noise
const int max_state_count = 3;

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
    // ratio the old value, combine with the new (reverse ratio)
    avg = avg * (1.0f - avg_ratio) + humidity * avg_ratio;
  }
  // don't use math inside max(), it causes undefined behaviour (see docs)
  int cur_deviation = humidity - avg;
  deviation = deviation * (1.0f - deviation_ratio) + cur_deviation * deviation_ratio;
  deviation = max(deviation, cur_deviation);
  thresholdLow = avg + deviation / 4.0f;
  thresholdHigh = avg + deviation / 2.0f;
  
  action = FAN_NOCHANGE;
  if (humidity < thresholdLow) {
    action = FAN_OFF;
  } else if (humidity > thresholdHigh) {
    action = FAN_ON;
  }
  
  if (action == new_state) {
    if (state_count < max_state_count) {
      state_count++;
    }
  } else {
    state_count = 1; // reset count
    new_state = action;
  }
  
  if (state_count == max_state_count) {
    // switch the relays!
    switch (new_state) {
      case FAN_OFF:
        // because we don't know the status of the 3-way switch, we switch both
        // relays at the same time and hope there is no short circuit. This should
        // not be an issue, but anyway...
        digitalWrite(K1, NormallyConnected);
        digitalWrite(K2, NormallyConnected);
        break;
      case FAN_ON:
        digitalWrite(K2, NormallyOpen);
        // since we don't know how the switch is switched, we first switch K2 to
        // non-connected, then wait a while for it to settle, before we switch
        // K1 to connected L. This should prevent any shorts (don't know if that
        // is actually an issue, but better safe than sorry).
        delay(milliseconds_switch_delay);
        digitalWrite(K1, NormallyOpen);
        break;
    }
    cur_state = new_state; // save the new state for displaying purposes
    state_count++; // increase it so we don't have to do this next time again
  }


  // display the current state in the graph
  // don't use absolute values, so the graph can zoom in and show more information
  on_off = avg;
  switch (cur_state) {
    case FAN_OFF:
      on_off -= 20;
      break;
    case FAN_ON:
      on_off -= 10;
      break;
    case FAN_NOCHANGE:
      on_off -= 15;
      break;
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
