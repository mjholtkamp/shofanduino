/* Shofanduino 2 DHT11, a shower fan arduino project with a digital humidity sensor
 *
 * Copyright (c) 2013, Michiel Holtkamp, see attached LICENSE file for licensing.
 *
 * I used a library from http://playground.arduino.cc/main/DHT11Lib
 *
 * For more information, see README
 */

#include <dht11.h> // see above for URLs
#include <runningmedian.h>

#define DHT11_PIN 23

// consts for controlling relays
const int K1 = 5;
const int K2 = 6;
const int milliseconds_switch_delay = 100;
const int NormallyConnected = HIGH;
const int NormallyOpen = LOW;
typedef enum {FAN_OFF, FAN_NOCHANGE, FAN_ON};

// DHT consts/variables
dht11 DHT11;

// consts/variables for humidity sensor and averages etc.
const int sensorPin = A0;    // select the input pin for the potentiometer
const int milliseconds_sleep = 5000;
const float min_humidity_diff= 10.0; // RH %
const float max_humidity = 100.0; // needed because we add min_humidity_diff

RunningMedian samples = RunningMedian(3);

const float thresholdHighRatio = 6.0f;
const float thresholdLowRatio = 16.0f;
float thresholdLow = 0.0;
float thresholdHigh = 0.0;
float env_min = 0.0;
float env_max = 0.0;
float humidity = 0.0;
float temperature = 0.0;
float on_off = 0;

// variables for decay
 // number of seconds after which the difference between the humidity and the max envelope should have been halved.
const float p_halflife_max = 604800.0;
//const float p_halflife_min = 86400.0; // same for the min envelope. This is shorter because it needs to change faster.
const float p_halflife_min = 43200.0; // same for the min envelope. This is shorter because it needs to change faster.
float env_max_decay = 0.0;
float env_min_decay = 0.0;

// variables to decide the state of the fan
int state = FAN_OFF; // action to take, based on sensor reading

void getValues() {
  // The sensor can only be read from every 1-2s, and requires a minimum
  // 2s warm-up after power-on.
  delay(milliseconds_sleep);

#ifdef USE_DHT22
  DHT22_ERROR_t errorCode = myDHT22.readData();
  humidity = myDHT22.getHumidity();
  temperature = myDHT22.getTemperatureC();
#else
  int errorCode = DHT11.read(DHT11_PIN);
  humidity = DHT11.humidity;
  temperature = DHT11.temperature;
#endif

  // clamp humidity
  humidity = max(humidity, 0);
  humidity = min(humidity, 100);
  samples.add(humidity);
  humidity = samples.getMedian();
  
  switch(errorCode) {
#ifdef USE_DHT22
  case DHT_ERROR_NONE:
    break;
  case DHT_ERROR_CHECKSUM:
    Serial.print("E: checksum ");
    Serial.print(myDHT22._lastCheckSum);
    Serial.print(" != ");
    Serial.print(myDHT22._lastExpectedCheckSum);
    Serial.print(". ");
    Serial.print(temperature);
    Serial.print("C ");
    Serial.print(humidity);
    Serial.println("%. Are you really sure you set the clock to 8Mhz?");
    break;
  case DHT_BUS_HUNG:
    Serial.println("E: BUS Hung");
    break;
  case DHT_ERROR_NOT_PRESENT:
    Serial.println("E: Sensor not found");
    break;
  case DHT_ERROR_ACK_TOO_LONG:
    Serial.println("E: ACK time out");
    break;
  case DHT_ERROR_SYNC_TIMEOUT:
    Serial.println("E: Sync Timeout");
    break;
  case DHT_ERROR_DATA_TIMEOUT:
    Serial.println("E: Data Timeout");
    break;
  case DHT_ERROR_TOOQUICK:
    Serial.println("E: Polled to quick");
    break; 
#else
  case DHTLIB_OK:
    break;
  case DHTLIB_ERROR_CHECKSUM:
    Serial.println("E: Checksum error");
    break;
  case DHTLIB_ERROR_TIMEOUT:
    Serial.println("E: timeout");
    break;
  default:
    Serial.println("E: unknown error");
    break;
#endif
  }
}

void setup() {
  Serial.begin(9600);

  // set relays to normally connected
  pinMode(K1, OUTPUT);
  pinMode(K2, OUTPUT);
  digitalWrite(K1, NormallyConnected);
  digitalWrite(K2, NormallyConnected);

  Serial.println("DHT11 temperature/humidity. Did you set the clock to 8Mhz?");

  // initialize sensor
  pinMode(sensorPin, INPUT);
  getValues();
  env_min = humidity;
  env_max = humidity + min_humidity_diff;
  env_max = min(env_max, max_humidity);
  
  // calculate decay
  float t = milliseconds_sleep / 1000.0f;

  // calculate what would happen after one step, to unit value (1.0)
  // the result is the scale factor we can apply to the envelope max/min
  env_max_decay = 1.0f * pow(2, -t / p_halflife_max);
  env_min_decay = 1.0f * pow(2, -t / p_halflife_min);
}

void loop() {
  getValues();

  /*
   * Calculate limits
   */
  
  // clamp the envelope min/max, decay otherwise
  if (env_min > humidity) {
    env_min = humidity;
  } else {
    // this actually decays the difference between humidity
    // and env_min, so this means env_min goes up!
    env_min = humidity - (humidity - env_min) * env_min_decay;
  }
  if (env_max < humidity) {
    env_max = humidity;
  } else {
    env_max = humidity + (env_max - humidity) * env_max_decay;
  }
  
  // New thresholds based on the envelope values. We
  // want the fan to turn on pretty quickly, but we don't
  // want it to turn off quickly. This explains the difference
  // in factors between low and high.
  thresholdLow = env_min + (env_max - env_min) / thresholdLowRatio;
  thresholdHigh = env_min + (env_max - env_min) / thresholdHighRatio;

  /*
   * Check state
   */
  int new_state = FAN_NOCHANGE;
  if (humidity < thresholdLow) {
    new_state = FAN_OFF;
  } else if (humidity > thresholdHigh) {
    new_state = FAN_ON;
  }

  if (new_state != state) {
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
  }
  state = new_state;

  // display the current state in the graph
  // don't use absolute values, so the graph can zoom in and show more information
  switch (state) {
    case FAN_OFF:
      on_off = 0.0;
      break;
    case FAN_ON:
      on_off = 1.0;
      break;
    case FAN_NOCHANGE:
      on_off = 0.5;
      break;
  }

  /*
   * Output for debugging/graphing
   */

  // print out the values for debugging/plotting
  Serial.print("humidity = ");
  Serial.print(humidity, 1);
  Serial.print(", threshold_low = ");
  Serial.print(thresholdLow, 3);
  Serial.print(", threshold_hi = ");
  Serial.print(thresholdHigh, 3);
  Serial.print(", envelope_min = ");
  Serial.print(env_min, 3);
  Serial.print(", envelope_max = ");
  Serial.print(env_max, 3);
  Serial.print(", temperature = ");
  Serial.print(temperature, 1);
  Serial.print(", on_off = ");
  Serial.println(on_off, 1);
}

