Shofanduino 2
=============

A shower fan arduino project with a digital humidity sensor.

Copyright (c) 2013, Michiel Holtkamp, see accompanying LICENSE file for
licensing.


Notes
-----

This version uses a digital humidity sensor. For a similar program that uses
an analogue sensor, see shofanduino (1).

I used a library from https://github.com/ringerc/Arduino-DHT22 which is forked
from the original: https://github.com/nethoncho/Arduino-DHT22 Both versions are
licensed LGPL.

For some reason, I have to run my Teensy2.0 (the arduino clone I used for this
project) on 8 MHz instead of 16 MHz, otherwise the timing of the code screwed
up the readings of the sensor. This works for me, since the code isn't very
demanding (a lower frequency would be fine as well, but again doesn't work
because of timing issues).


How it works
------------

Shofanduino 2 measures the relative humidity in a bathroom and turns on the air
fan when someone takes a shower. When the humidity has dropped sufficiently,
the air fan is turned off automatically.

The sensor used is a DHT22 Relative Humidity/Temperature sensor. The
temparature is also read (and displayed for debugging), but is not used to
decide to turn the fan on or off. The first version of Shofanduino used an
analogue sensor, and this code is based on that. However, since the digital
sensor does a lot of noise filtering, this code is much simpler (because part
of the work is now done by the sensor).


Automatic minimum/maximum
-------------------------

Shofanduino automatically calculates thresholds for turning the fan on and off,
so it's not necessary to configure any settings. It makes a few assumptions
though. The first assumption is that the sensor is located in the bathroom
itself.  Placing the sensor further from the shower creates a slower reaction
time and possibly a faster decision to switch off the shower fan.

The tresholds are calculated by recording the minimum and maximum of the
(relative) humidity value. To account for yearly (or maybe weekly) drift of
base humidity values, the extrema are decayed in the direction of the current
humidity value, making the difference between them smaller over time. Whenever
the current value drops below the minimum (or peaks above the maximum), the
minimum (or maximum) is adjusted immediately, so the minimum is never greater
than the current value and the maximum is never smaller than the current value.

The decays of the extrema are calculated with the same equation that is used
for half-life calculations: P_t = P_0 * 2^{ -t / t_hl }, where:
 - t is the time
 - P_t is the concentration at time t,
 - P_0 is the concentration at time 0,
 - t_hl is the half-time of the substance.

For the minimum, I chose a half-time of a day, meaning that the difference
between the minimum and the current humidity is halved each day. For the
maximum, I chose a half-time of a week, so consequently after a week the
difference between the maximum and the current humidity value is halved. This
of course assumes that the current humidity value stays the same during this
period, which it doesn't.

The reason why the half-life is different for the two extrema, is because the
current humidity value should be closer to the minumum most of the time, while
the humidity value should only be close to the maximum during showers.  This of
course assumes that the shower is on only a small part of the time.

To use half-life decay of the extrema, we calculate the decay for a unit value
(1.0) for our delay between measurements (e.g. 5 seconds) and use that factor
to decay the extrema each measurement. Due to round-off errors, the difference
between direct calculation and numerical estimation will accumulate. Assuming a
5 second measurement interval, the error is about 0.1% after one week (see the
param_research program, run it on an arduino). Meaning that for the full range
of relative humidity (0% - 100%), starting at 100%, the value after a week will
be 50.0% with the direct calculation and will be about 49.9% with the numerical
estimation.

This error is acceptable because the above example is an extreme case, the
sensor precision is also 0.1% and this error is after a full week of running.
Since the humidity will constantly change, this error will not be noticable,
let alone have a big impact on the final result.


Calculating thresholds from the extrema
---------------------------------------

From the extrema, the lower and higher thresholds are calculated each
measurement.  When the value rises above the high threshold, the fan is turned
on and when the value drops below the low threshold, the fan is turned off. Two
thresholds are used to prevent flapping (quickly turning the fan on/off/on/off
because the humidity value is oscillating around a single threshold). I chose
the high threshold as half of the distance between the extrema and I chose the
low threshold as 1/20th of that distance (the low threshold is closest to the
minimum).

These values were chosen by experiments with the first sensor. 1/20th may be a
little bit too small, I might change it to 1/10th in the future. I expect it
will be ok, since if the current humidity value will stay above the lower
threshold, the minimum will decay towards it, pushing the low threshold higher,
thus effectively lowering the current humidity value below the low threshold.
Future will have to show how this actually works out.


Final steps
-----------

After all of this is done, relays are switch on or off, depending on if the
value of the current humidity is above or below the thresholds.

The final step is to display the measeruments over the serial line. This is
only used for debugging/graphing, as this program is designed to run
independently from a PC, meaning that you can just power the arduino with USB
or a different power source.
