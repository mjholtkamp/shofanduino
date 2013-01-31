/* Shofanduino 2, a shower fan arduino project with a digital humidity sensor
 *
 * Copyright (c) 2013, Michiel Holtkamp, see attached LICENSE file for licensing.
 *
 * I used a library from https://github.com/ringerc/Arduino-DHT22 which is forked
 * from the original: https://github.com/nethoncho/Arduino-DHT22 Both versions are
 * licensed LGPL.
 *
 * For some reason, I have to run my Teensy2.0 (the arduino clone I used for this
 * project) on 8 MHz instead of 16 MHz, otherwise the timing of the code screwed
 * up the readings of the sensor. This works for me, since the code isn't very
 * demanding (a lower frequency would be fine as well, but again doesn't work
 * because of timing issues).
 *
 * Shofanduino 2 measures the relative humidity in a bathroom and turns on the
 * air fan when someone takes a shower. When the humidity has dropped sufficiently,
 * the air fan is turned off automatically.
 *
 * The sensor used is a DHT22 Relative Humidity/Temperature sensor. The temparature
 * is also read (and displayed for debugging), but is not used to decide to turn
 * the fan on or off. The first version of Shofanduino used an analogue sensor,
 * and this code is based on that. However, since the digital sensor does a lot of
 * noise filtering, this code is much simpler (because part of the work is now done
 * by the sensor).
 *
 * Shofanduino automatically calculates thresholds for turning the fan on and off,
 * so it's not necessary to configure any settings. It makes a few assumptions
 * though. The first assumption is that the sensor is located in the bathroom itself.
 * Placing the sensor further from the shower creates a slower reaction time and
 * possibly a faster decision to switch off the shower fan.
 *
 * The tresholds are calculated by recording the minimum and maximum of the (relative)
 * humidity value. To account for yearly (or maybe weekly) drift of base humidity
 * values, the extrema are decayed in the direction of the current humidity value,
 * making the difference between them smaller over time. Whenever the current value
 * drops below the minimum (or peaks above the maximum), the minimum (or maximum)
 * is adjusted immediately, so the minimum is never greater than the current value
 * and the maximum is never smaller than the current value.
 *
 * The decays of the extrema are calculated with the same equation that is used for
 * half-life calculations: P_t = P_0 * 2^{ -t / t_hl }, where:
 *  - t is the time
 *  - P_t is the concentration at time t,
 *  - P_0 is the concentration at time 0,
 *  - t_hl is the half-time of the substance.
 *
 * For the minimum, I chose a half-time of a day, meaning that the difference
 * between the minimum and the current humidity is halved each day. For the maximum,
 * I chose a half-time of a week, so consequently after a week the difference
 * between the maximum and the current humidity value is halved. This of course
 * assumes that the current humidity value stays the same during this period, which
 * it doesn't. The reason why the half-life is different for the two extrema, is
 * because the current humidity value should be closer to the minumum most of the
 * time, while the humidity value should only be close to the maximum during showers.
 * This of course assumes that the shower is on only a small part of the time.
 *
 * To use half-life decay of the extrema, we calculate the decay for a unit value (1.0)
 * for our delay between measurements (e.g. 5 seconds) and use that factor to decay
 * the extrema each measurement. Due to round-off errors, the difference between
 * direct calculation and numerical estimation will accumulate. Assuming a 5 second
 * measurement interval, the error is about 0.1% after one week (see the param_research
 * program, run it on an arduino). Meaning that for the full range of relative humidity
 * (0% - 100%), starting at 100%, the value after a week will be 50.0% with the direct
 * calculation and will be about 49.9% with the numerical estimation. This error is
 * acceptable because the above example is an extreme case, the sensor precision is also
 * 0.1% and this error is after a full week of running. Since the humidity will constantly
 * change, this error will not be noticable, let alone have a big impact on the final
 * result.
 *
 * From the extrema, the lower and higher thresholds are calculated each measurement.
 * When the value rises above the high threshold, the fan is turned on and when the value
 * drops below the low threshold, the fan is turned off. Two thresholds are used to
 * prevent flapping (quickly turning the fan on/off/on/off because the humidity value
 * is oscillating around a single threshold). I chose the high threshold as half of
 * the distance between the extrema and I chose the low threshold as 1/20th of that
 * distance (the low threshold is closest to the minimum). These values were chosen by
 * experiments with the first sensor. 1/20th may be a little bit too small, I might
 * change it to 1/10th in the future. I expect it will be ok, since if the current humidity
 * value will stay above the lower threshold, the minimum will decay towards it, pushing
 * the low threshold higher, thus effectively lowering the current humidity value below
 * the low threshold. Future will have to show how this actually works out.
 *
 * After all of this is done, relays are switch on or off, depending on if the value
 * of the current humidity is above or below the thresholds.
 *
 * The final step is to display the measeruments over the serial line. This is only
 * used for debugging/graphing, as this program is designed to run independently from
 * a PC, meaning that you can just power the arduino with USB or a different power source.
 */

#include <DHT22.h> // see above for URLs

#define DHT22_PIN 6

// consts for controlling relays
const int K1 = 11;
const int K2 = 12;
const int milliseconds_switch_delay = 100;
const int NormallyConnected = HIGH;
const int NormallyOpen = LOW;
typedef enum {FAN_OFF, FAN_NOCHANGE, FAN_ON};

// DHT consts/variables
DHT22 myDHT22(DHT22_PIN);

// consts/variables for humidity sensor and averages etc.
const int sensorPin = A0;    // select the input pin for the potentiometer
const int milliseconds_sleep = 5000;
const float min_humidity_diff= 10.0; // RH %
const float max_humidity = 100.0; // needed because we add min_humidity_diff

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
const float p_halflife_min = 86400.0; // same for the min envelope. This is shorter because it needs to change faster.
float env_max_decay = 0.0;
float env_min_decay = 0.0;

// variables to decide the state of the fan
int state = FAN_OFF; // action to take, based on sensor reading

void getValues() {
  DHT22_ERROR_t errorCode;

  // The sensor can only be read from every 1-2s, and requires a minimum
  // 2s warm-up after power-on.
  delay(milliseconds_sleep);

  errorCode = myDHT22.readData();
  humidity = myDHT22.getHumidity();
  temperature = myDHT22.getTemperatureC();

  switch(errorCode) {
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
  }
}

void setup() {
  Serial.begin(9600);

  // set relays to normally connected
  pinMode(K1, OUTPUT);
  pinMode(K2, OUTPUT);
  digitalWrite(K1, NormallyConnected);
  digitalWrite(K2, NormallyConnected);

  Serial.println("DHT22 temperature/humidity. Did you set the clock to 8Mhz?");

  // initialize sensor
  pinMode(sensorPin, INPUT);
  getValues();
  env_min = humidity;
  env_max = humidity;
  
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
  float humidity_margin = humidity + min_humidity_diff;
  humidity_margin = min(humidity_margin, max_humidity);

  if (env_max < humidity_margin) {
    env_max = humidity_margin;
  } else {
    env_max = humidity_margin + (env_max - humidity_margin) * env_max_decay;
  }
  
  // New thresholds based on the envelope values. We
  // want the fan to turn on pretty quickly, but we don't
  // want it to turn off quickly. This explains the difference
  // in factors between low and high.
  thresholdLow = env_min + (env_max - env_min) / 20.0f;
  thresholdHigh = env_min + (env_max - env_min) / 2.0f;

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
    switch (state) {
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

