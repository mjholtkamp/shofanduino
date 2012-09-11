#!/usr/bin/python
import time
import random
import math

minute = 60
hour = 3600
day = hour * 24
showertime = 14 * hour
week = 7 * day
month = 30 * day
year = 365 * day
resolution = 5

def generate_humidity(period):
	start = int(time.time())

	showerfree_start = period / 4
	showerfree_stop = showerfree_start + period / 4

	shower_humidity = 0
	initial_base_humidity = 50

	for seconds in xrange(start, start + period, resolution):
		base_humidity = random.normalvariate(165, 1) + initial_base_humidity

		# the sensor has some startup deviation, it will settle after a while
		initial_base_humidity *= 0.98
		period_humidity = math.sin((float(seconds) / year) * 2 * math.pi) * 10.0
		if (seconds - (seconds % resolution)) % day == showertime:
			if seconds - start < showerfree_start or seconds - start > showerfree_stop:
				shower_humidity = 50

#		if seconds == start + period / 2:
#			base_humidity += 100

		shower_humidity *= 0.995
		humidity = base_humidity + shower_humidity + period_humidity

		yield (seconds, humidity)

f = open('humidity.data', 'w')
f.write('# seconds humidity deviation avg low high\n')

############################################
avg_ratio = 0.00002
deviation_ratio = 0.000001
avg = -1
deviation = 20
startup_seconds = 15 * minute

total_duration = 7 * week
data_points = total_duration / resolution # this many data points
wanted_data_points = 2000
print_resolution = resolution * (data_points / wanted_data_points)

# initialise humidity, first sample
for (seconds, humidity) in generate_humidity(resolution):
	avg = humidity

print_seconds = print_resolution # print the first one immediately
for (seconds, humidity) in generate_humidity(total_duration):
	if startup_seconds > 0:
		avg = (avg + humidity) / 2
		startup_seconds -= 5
	else:
		avg = avg * (1 - avg_ratio) + humidity * avg_ratio

	deviation = deviation * (1 - deviation_ratio) + (humidity - avg) * deviation_ratio
	deviation = max(deviation, humidity - avg)
	t_lo = avg + deviation / 3
	t_hi = avg + 2 * deviation / 3
	print_seconds += resolution
	if print_seconds > print_resolution:
		f.write('%u %g %g %g %g %g\n' % (seconds, humidity, avg + deviation, avg, t_lo, t_hi))
		print_seconds = 0
