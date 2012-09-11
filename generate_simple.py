#!/usr/bin/python
import time
import random
import math

minute = 60
hour = 3600
day = hour * 24
showertime = 13 * hour
week = 7 * day
month = 30 * day
year = 365 * day

def generate_humidity(period):
	start = int(time.time())

	showerfree_start = period / 4
	showerfree_stop = showerfree_start + period / 4

	shower_humidity = 0
	initial_base_humidity = 50

	step = 5
	for seconds in xrange(start, start + period, step):
		base_humidity = random.normalvariate(165, 1) + initial_base_humidity

		# the sensor has some startup deviation, it will settle after a while
		initial_base_humidity *= 0.98
		period_humidity = math.sin((float(seconds) / week) * 2 * math.pi) * 10.0
		if (seconds - (seconds % step)) % day == showertime:
	#		if seconds - start < showerfree_start or seconds - start > showerfree_stop:
				shower_humidity = 50

#		if seconds == start + period / 2:
#			base_humidity += 100

		shower_humidity *= 0.995
		humidity = base_humidity + shower_humidity + period_humidity

		yield (seconds, humidity)

f = open('humidity.data', 'w')
f.write('# seconds humidity deviation avg low high\n')

############################################
avg_decay = 0.0002
deviation_decay = 0.01
avg = -1
deviation = 20
startup_seconds = 15 * minute

# initialise humidity, first sample
for (seconds, humidity) in generate_humidity(5):
	avg = humidity

for (seconds, humidity) in generate_humidity(7 * week):
	if startup_seconds > 0:
		avg = (avg + humidity) / 2
		startup_seconds -= 5
	else:
		avg = avg * (1 - avg_decay) + humidity * avg_decay

	cur_deviation = max(deviation, humidity - avg)
	deviation = deviation * (1 - deviation_decay) + cur_deviation * deviation_decay
	t_lo = avg + deviation / 3
	t_hi = avg + 2 * deviation / 3
	f.write('%u %g %g %g %g %g\n' % (seconds, humidity, avg + deviation, avg, t_lo, t_hi))
