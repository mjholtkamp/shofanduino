#!/usr/bin/python
import time
import random
import math

fivesec = 5
fivemin = 300
hour = 3600
day = hour * 24
showertime = 2 * day / 3
week = 7 * day
month = 30 * day
year = 365 * day

def generate_humidity(period):
	start = int(time.time())

	showerfree_start = period / 4
	showerfree_stop = showerfree_start + period / 4

	shower_humidity = 0
	for seconds in xrange(start, start + period, fivesec):
		base_humidity = random.normalvariate(165, 1)
		period_humidity = math.sin((float(seconds) / year) * 2 * math.pi) * 10.0
		if (seconds - (seconds % fivemin)) % day == showertime:
			if seconds - start < showerfree_start or seconds - start > showerfree_stop:
				shower_humidity = 50

		if seconds == start + period / 2:
			base_humidity += 100

		shower_humidity *= 0.995
		humidity = base_humidity + shower_humidity + period_humidity

		yield (seconds, humidity)

f = open('humidity.data', 'w')
f.write('# seconds humidity max min low high\n')

############################################
max_humidity = 0
min_humidity = 1023
avg_decay = 0.00001
avg = -1
deviation = 20
deviation_decay = 0.01

for (seconds, humidity) in generate_humidity(5 * hour):
	if avg == -1:
		avg = humidity

	avg = avg * (1 - avg_decay) + humidity * avg_decay
	cur_deviation = max(deviation, humidity - avg)
	deviation = deviation * (1 - deviation_decay) + cur_deviation * deviation_decay
	t_lo = avg + deviation / 3
	t_hi = avg + 2 * deviation / 3
	f.write('%u %g %g %g %g %g\n' % (seconds, humidity, avg + deviation, avg, t_lo, t_hi))
