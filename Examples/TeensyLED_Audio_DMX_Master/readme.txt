TeensyLED Audio DMX Master
Version 0.9
Copyright Brian Neltner 2016

Video Example Demonstrating Algorithm Functionality with Color Kinetics ColorBlast 12.
https://www.youtube.com/watch?v=BOnsDaA30Ak

Description:

This program is designed for use with the SaikoLED TeensyLED Driver
system, using the ADM2582E from Analog Devices to provide a fully
isolated RS485/DMX-512 interface which has been demonstrated as both
a DMX master and DMX slave device.

TeensyLED Board Design Files (EagleCAD) and some other libraries can
be found at:
https://github.com/saikoLED/TeensyLED

A jumper is available on the TeensyLED board to allow switching between
the formal DMX-512a spec for DMX over RJ45/CAT5 cabling and the Color
Kinetics version of the standard, which switches DATA- and DATA+ and the
shield/ground connections.

Reference:
https://en.wikipedia.org/wiki/DMX512#RJ-45_pinout

This program assumes that line level audio (~1V peak-to-peak) is
available, and wired to analog input 0 on the Teensy 3.1 (on the
TeensyLED, this is available on JP1. In this example, A0 is connected
to 3.3V and GND via 100k resistors to provide a mid-scale DC offset
and then a capacitor is used to connect the audio input to AO.

As such, you end up with a capacitively coupled audio signal centered
at mid-scale. There are other ways to accomplish this with higher
fidelity, but for this purpose the audio quality is not particularly
important as it is never output to speakers and is only used for
controlling the lighting.

The basic algorithm below utilizes the SaikoLED HSI to RGB system to
convert a hue-saturation-intensity color to RGB which is then output
to standard RGB lighting. RGBW lighting could be used by switching
to the SaikoLED HSI->RGBW code, or using the new full CIE color
correction algorithm specifically designed to allow the TeensyLED to
control many-wavelength devices such as the LEDEngin LZ7 series LED
that provides RAGCBVW LEDs.

LEDEngin LZ7 LED and CIE Color Correction.
http://blog.saikoled.com/post/133625978643/full-cie-color-correction-with-the-teensy-31

For this audio analysis effect, the hue of the LED light is constantly
rotating, with beat detection utilized to cause pulses in the lighting.
Volume normalization is accomplished by saving the maximum audio signal
seen when the signal exceeds the prior maximum, but then gradually
letting the max signal level decay back to a low value over time so that
it can both immediately respond to volume increases, and recognize
when the audio has gotten quieter.

This effect also accomplishes automatic thresholding for the derivitive
of the audio signal for beat detection -- you select a target estimate
for the number of beats per second you'd like to see, and it gradually
tunes the threshold to the actual audio received to achieve roughly that
number of detection events. This works effectively to allow the user
to switch between very different music genres without needing to reprogram
a pile of magic constants to get it to look good.

Upon beat detection, the intensity of the lights immediately increases to
the normalized volume seen during that beat. As such, a loud beat in a soft
section of music will cause the light to flash brightly while a soft beat
in a loud section will appear more as a subtle flash. This results in a 
very nice effect wherein a loud beat followed by several soft accent beats
(very common in music of all kinds) shows the complexities of the beat
pattern very intuitively.

After a beat detection, the light brightness gradually decays -- the light
turning on is immediate but the decay takes ~100ms which gives a nice shape
that matches most drum signals very naturally. This decay time also varies
automatically based on the number of beats in the song. If a song has many
many beats the decay time is short so that the beats appear visually cleaner
to match the feeling of the music.

In this example code, I demonstrate controlling 8 DMX lights which are
arranged in a color wheel so that the entire array rotates around while
pulsing synchronously.

This rough algorithm was first demonstrated in 2013:
http://blog.saikoled.com/post/44823088119/myki-prototype-with-direct-audio-analysis
and the improved algorithm below was first demonstrated in 2016 at the
Firefly Arts Festival.

This software requires the DmxSimple library which uses bit-banging of
DMX outputs and does not support RDM or other advanced DMX features.
However, the ADM2582E and Teensy 3.1 should be perfectly capable of using
these features if a better library becomes available.

Filtering is used throughout this code, using explonential smoothing as a
very fast and simple way to accomplish a low-pass filter.

Reference:
https://en.wikipedia.org/wiki/Exponential_smoothing

An example is:

audioZero = 0.999999*audioZero + 0.000001*audioSignal;

which takes the prior zero and mixes it with the instantaneous
audio value to get a low-pass filtered zero. The two constants must
always add to one, but the larger the 0.000001 value is the more
quickly the filtered version will adjust to changes.

I am not good at C flags, but there are commented out serial communication
lines that can be uncommented for debug information (for instance, printing
the audio RMS values and beat detection values for tuning and debugging
audio signal issues). Some of this should probably be better encapsulated
in a C++ style class to reduce the unabstracted complexity of the effects.

License:

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
