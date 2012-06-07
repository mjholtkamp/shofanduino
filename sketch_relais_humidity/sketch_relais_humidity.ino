#include <Time.h>

const int relaisPin =  11;
const int sensorPin = A0;    // select the input pin for the potentiometer
const int MINVALUE = 0;
const int MAXVALUE = 1023;
int sensorValue = 0;  // variable to store the value coming from the sensor
int thresholdLow = 0;
int thresholdHigh = 0;
int hourMax = MINVALUE;
int hourMin = MAXVALUE;
int prevweekday = 0, prevhour = 0;
int curweekday = 0, curhour = 0;

/* Three arrays that keep track of sensor values (one every 5 minutes),
 * and averages (24 hours a day, then 7 days in a week)
 */
struct sensordata {
  int average;
  int minimum;
  int maximum;
};
typedef struct sensordata sensordata;

const int hum_hour_max = 12; // 12 * 5 minutes in one hour
const int hum_day_max = 24; // hours in a day
const int hum_week_max = 7; // days in a week
sensordata hum_hour[hum_hour_max];
sensordata hum_day[hum_day_max];
sensordata hum_week[hum_week_max];
int hum_hour_len = 0;
int hum_day_len = 0;
int hum_week_len = 0;

sensordata average;

int humidity() {
  sensorValue = analogRead(sensorPin);
  
  // sensor value is high when humidity is low, this is
  // counterintuitive (it's a dryness sensor!), so we
  // reverse it:
  return MAXVALUE - sensorValue;
}

void avg_min_max(const struct sensordata *sdata, int length, struct sensordata *out) {
  out->average = 0;
  out->minimum = MAXVALUE;
  out->maximum = MINVALUE;

  for (int i = 0; i < length; ++i) {
    out->average =+ sdata[i].average;
    out->minimum = min(sdata[i].minimum, out->minimum);
    out->maximum = max(sdata[i].maximum, out->maximum);
  }
  out->average /= length;
}

void setup() {
  // initialize the 11 pin as an output:
  Serial.begin(9600);
  pinMode(relaisPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(relaisPin, HIGH);
  
  prevweekday = weekday();
  prevhour = hour();
  sensorValue = humidity();
  // first run, initialize threshold to sane values
  average.average = sensorValue;
  average.minimum = -10;
  average.maximum = 10;
  thresholdLow = constrain(sensorValue + average.minimum, 0, MAXVALUE);
  thresholdHigh = constrain(sensorValue + average.maximum, 0, MAXVALUE);
}

void loop() {
  sensorValue = humidity();
  hourMin = constrain(sensorValue, 0, hourMin);
  hourMax = constrain(sensorValue, hourMax, MAXVALUE);
  sensordata tmp = {sensorValue, hourMin - sensorValue, hourMax - sensorValue};
  hum_hour[hum_hour_len] = tmp;
  if (hum_hour_len < hum_hour_max - 1) {
    hum_hour_len++;
  }
  
  curweekday = weekday();
  if (prevweekday != curweekday) {
    // every start of the week, calculate the averages of the week before
    if (curweekday == 0) {
      avg_min_max(hum_week, hum_week_len, &average);
      hum_week_len = 0;
    }
    // every start of the day, calculate the averages of the day before
    avg_min_max(hum_day, hum_day_len, &hum_week[hum_week_len]);
    hum_day_len = 0;
    if (hum_week_len < hum_week_max - 1) {
      hum_week_len++;
    }
    prevweekday = curweekday;
  }
  
  // every start of the hour, calculate the averages of the 5 minute periods
  curhour = hour();
  if (prevhour != curhour) {
    prevhour = curhour;
    avg_min_max(hum_hour, hum_hour_len, &hum_day[hum_day_len]);
    hum_hour_len = 0;
    if (hum_day_len < hum_day_max - 1) {
      hum_day_len++;
    }
    
    average.maximum = max(average.maximum, hourMax - sensorValue);
    thresholdHigh = average.average + (average.maximum / 4) * 3;
    thresholdLow = average.average + average.maximum / 4;
    
    hourMin = MAXVALUE;
    hourMax = MINVALUE;
  }

  // print out the values for debugging/plotting
  Serial.print("humidity = ");
  Serial.print(sensorValue);
  Serial.print(", tlow = ");
  Serial.print(thresholdLow);
  Serial.print(", thi = ");
  Serial.print(thresholdHigh);
  Serial.print(", dmin = ");
  Serial.print(average.average + average.minimum);
  Serial.print(", dmax = ");
  Serial.println(average.average + average.maximum);

  // switch the relays!
  if (sensorValue < thresholdLow) {
    // HIGH means setting the relay to NC
    digitalWrite(relaisPin, HIGH);
  } else if (sensorValue > thresholdHigh) {
    // LOW means setting the relay to NO
    digitalWrite(relaisPin, LOW); 
  }
  delay(1000);
}
