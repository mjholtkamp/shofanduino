#!/usr/bin/python
import time
import random
import math

fivemin = 300
hour = 3600
day = hour * 24
showertime = day / 3
week = 7 * day
month = 30 * day
year = 365 * day

def generate_humidity(period):
	start = int(time.time())

	showerfree_start = period / 4
	showerfree_stop = showerfree_start + 1 * week

	shower_humidity = 0
	for seconds in xrange(start, start + period, fivemin):
		base_humidity = random.normalvariate(165, 1)
		period_humidity = math.sin((float(seconds) / year) * 2 * math.pi) * 10.0
		if (seconds - (seconds % fivemin)) % day == showertime:
			if seconds - start < showerfree_start or seconds - start > showerfree_stop:
				shower_humidity = 50

		shower_humidity *= 0.8
		humidity = base_humidity + shower_humidity + period_humidity

		yield (seconds, humidity)

f = open('humidity.data', 'w')
f.write('# seconds humidity max min low high\n')

def avg_min_max(array, length):
	avg = sum([x[0] for x in array[:length]]) / length
	_min = min([x[1] for x in array[:length]])
	_max = max([x[2] for x in array[:length]])
	return (avg, _min, _max)

############################################
hum_hour_max = 12 # 12 * 5 minutes in one hour
hum_day_max = 24 # averaged hours a day
hum_week_max = 7 # averaged days a week
hum_hour = [(0,0,0)] * hum_hour_max
hum_day = [(0,0,0)] * hum_day_max
hum_week = [(0,0,0)] * hum_week_max
hum_hour_len = 0
hum_day_len = 0
hum_week_len = 0

############################################
max_humidity = 0
min_humidity = 1023
average = (165, -10, 10)
t_lo = average[0] + average[1]
t_hi = average[0] + average[2]
for (seconds, humidity) in generate_humidity(3 * month):
	max_humidity = max(max_humidity, humidity)
	min_humidity = min(min_humidity, humidity)

	hum_hour[hum_hour_len] = (humidity, min_humidity - humidity, max_humidity - humidity)
	if hum_hour_len < hum_hour_max - 1:
		hum_hour_len += 1

	# every start of the week
	if (seconds - (seconds % fivemin)) % week == 0:
		average = avg_min_max(hum_week, hum_week_len)
		hum_week_len = 0

	# every start of the day
	if (seconds - (seconds % fivemin)) % day == 0:
		hum_week[hum_week_len] = avg_min_max(hum_day, hum_day_len)
		hum_day_len = 0
		if hum_week_len < hum_week_max - 1:
			hum_week_len += 1

	# every start of the hour
	if (seconds - (seconds % fivemin)) % hour == 0:
		hum_day[hum_day_len] = avg_min_max(hum_hour, hum_hour_len)
		hum_hour_len = 0
		if hum_day_len < hum_day_max - 1:
			hum_day_len += 1

		average = (average[0], average[1], max(average[2], max_humidity - humidity))
		diff = average[2]
		t_lo = average[0] + diff / 4
		t_hi = average[0] + (diff / 4) * 3

		max_humidity = 0
		min_humidity = 1023

	f.write('%u %g %g %g %g %g\n' % (seconds, humidity, average[0] + average[2], average[0] + average[1], t_lo, t_hi))
