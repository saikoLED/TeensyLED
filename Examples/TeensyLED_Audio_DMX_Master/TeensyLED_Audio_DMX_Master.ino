// ----------------------------------------------------------------------
//
// TeensyLED Audio DMX Master
// Version 0.9
// Copyright Brian Neltner 2016
//
// Description:
//
// This program is designed for use with the SaikoLED TeensyLED Driver
// system, using the ADM2582E from Analog Devices to provide a fully
// isolated RS485/DMX-512 interface which has been demonstrated as both
// a DMX master and DMX slave device.
//
// TeensyLED Board Design Files (EagleCAD) and some other libraries can
// be found at:
// https://github.com/saikoLED/TeensyLED
//
// A jumper is available on the TeensyLED board to allow switching between
// the formal DMX-512a spec for DMX over RJ45/CAT5 cabling and the Color
// Kinetics version of the standard, which switches DATA- and DATA+ and the
// shield/ground connections.
//
// Reference:
// https://en.wikipedia.org/wiki/DMX512#RJ-45_pinout
//
// This program assumes that line level audio (~1V peak-to-peak) is
// available, and wired to analog input 0 on the Teensy 3.1 (on the
// TeensyLED, this is available on JP1. In this example, A0 is connected
// to 3.3V and GND via 100k resistors to provide a mid-scale DC offset
// and then a capacitor is used to connect the audio input to AO.
//
// As such, you end up with a capacitively coupled audio signal centered
// at mid-scale. There are other ways to accomplish this with higher
// fidelity, but for this purpose the audio quality is not particularly
// important as it is never output to speakers and is only used for
// controlling the lighting.
//
// The basic algorithm below utilizes the SaikoLED HSI to RGB system to
// convert a hue-saturation-intensity color to RGB which is then output
// to standard RGB lighting. RGBW lighting could be used by switching
// to the SaikoLED HSI->RGBW code, or using the new full CIE color
// correction algorithm specifically designed to allow the TeensyLED to
// control many-wavelength devices such as the LEDEngin LZ7 series LED
// that provides RAGCBVW LEDs.
//
// LEDEngin LZ7 LED and CIE Color Correction.
// http://blog.saikoled.com/post/133625978643/full-cie-color-correction-with-the-teensy-31
//
// For this audio analysis effect, the hue of the LED light is constantly
// rotating, with beat detection utilized to cause pulses in the lighting.
// Volume normalization is accomplished by saving the maximum audio signal
// seen when the signal exceeds the prior maximum, but then gradually
// letting the max signal level decay back to a low value over time so that
// it can both immediately respond to volume increases, and recognize
// when the audio has gotten quieter.
//
// This effect also accomplishes automatic thresholding for the derivitive
// of the audio signal for beat detection -- you select a target estimate
// for the number of beats per second you'd like to see, and it gradually
// tunes the threshold to the actual audio received to achieve roughly that
// number of detection events. This works effectively to allow the user
// to switch between very different music genres without needing to reprogram
// a pile of magic constants to get it to look good.
//
// Upon beat detection, the intensity of the lights immediately increases to
// the normalized volume seen during that beat. As such, a loud beat in a soft
// section of music will cause the light to flash brightly while a soft beat
// in a loud section will appear more as a subtle flash. This results in a 
// very nice effect wherein a loud beat followed by several soft accent beats
// (very common in music of all kinds) shows the complexities of the beat
// pattern very intuitively.
//
// After a beat detection, the light brightness gradually decays -- the light
// turning on is immediate but the decay takes ~100ms which gives a nice shape
// that matches most drum signals very naturally. This decay time also varies
// automatically based on the number of beats in the song. If a song has many
// many beats the decay time is short so that the beats appear visually cleaner
// to match the feeling of the music.
//
// In this example code, I demonstrate controlling 8 DMX lights which are
// arranged in a color wheel so that the entire array rotates around while
// pulsing synchronously.
//
// This rough algorithm was first demonstrated in 2013:
// http://blog.saikoled.com/post/44823088119/myki-prototype-with-direct-audio-analysis
// and the improved algorithm below was first demonstrated in 2016 at the
// Firefly Arts Festival.
//
// This software requires the DmxSimple library which uses bit-banging of
// DMX outputs and does not support RDM or other advanced DMX features.
// However, the ADM2582E and Teensy 3.1 should be perfectly capable of using
// these features if a better library becomes available.
//
// Filtering is used throughout this code, using explonential smoothing as a
// very fast and simple way to accomplish a low-pass filter.
//
// Reference:
// https://en.wikipedia.org/wiki/Exponential_smoothing
//
// An example is:
//
// audioZero = 0.999999*audioZero + 0.000001*audioSignal;
//
// which takes the prior zero and mixes it with the instantaneous
// audio value to get a low-pass filtered zero. The two constants must
// always add to one, but the larger the 0.000001 value is the more
// quickly the filtered version will adjust to changes.
//
// I am not good at C flags, but there are commented out serial communication
// lines that can be uncommented for debug information (for instance, printing
// the audio RMS values and beat detection values for tuning and debugging
// audio signal issues). Some of this should probably be better encapsulated
// in a C++ style class to reduce the unabstracted complexity of the effects.
//
// License:
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------

#include <DmxSimple.h>

// Starting points for the audio DC offset, the lowest allowable audioRMSMax value,
// the starting threshold for a delta audio to be considered a beat, the minimum
// LED intensity, the target number of beats per second, the timestep between
// light setpoint updates, and the amount of hue change per second in degrees.

#define audioZeroStart 4096
#define audioRMSMaxStart 100
#define dThresholdStart 10
#define dThresholdMin 1
#define intensityMin 0.05
#define lightDecayPeriodStart 100
#define bpsTarget 6
#define timestep 10
#define hueStepPerSecond 60

// The initial hues of each light. In this example there are eight
// RGB lights in the DMX universe numbered in the typical fashion
// with the first light using DMX Channel 0 for red, Channel 1 for
// green, Channel 2 for blue, and then the second light doing the
// same starting with DMX Channel 3 for red. I used here fully
// saturated colors, so did not need to keep track of the saturation.

float huearray[8] = {0, 45, 90, 135, 180, 225, 270, 315};
float intensityarray[8] = {intensityMin, intensityMin, intensityMin, intensityMin, intensityMin, intensityMin, intensityMin, intensityMin};

// Flag to indicate that a beat has been detected. Beats are only
// recognized every DMX update (10ms default) which incidentally also
// manages debouncing to avoid confusing visual effects.

boolean beatdetected;

// Global variables to store filtered versions of signals and beat counts for
// auto-tuning. Also has decay periods for LED turn-off and the beat detection
// threshold which are varied slightly to adjust for different music genres.

float daudioRMSFiltered;
float dThreshold;
unsigned int beatCounts;
float beatCountsFiltered;
float lightDecayPeriod;
float audioRMSFiltered;
float audioRMSMax;
float audioZero;
float peakBeatVolume;

// Set up a timer for audio sampling rate. 44.1kHz is roughly 22us.
// In all honesty, this high of a speed is unnecessary to get excellent
// results, but your filter time constants will need changing to
// match different sampling rates.

elapsedMicros audioSamplingTimer;

// Set up a timer to periodically see how we're doing on getting a
// pleasant number of light flashes per beat analysis period.

elapsedMillis beatTimer;

// Set up a timer for actually sending updates to LED lights.

elapsedMillis sendtimer;

void setup() {
  // I like to delay for a second so that serial is up and running before
  // the code starts in earnest.
  
  delay(1000);
  Serial.println("TeensyLED Audio Analysis System Operational.");
  
  // Setup for the DMX Simple Library. Pin 1 is the TX pin on the Teensy.
  // A better DMX library would allow me to send packets using the hardware
  // UART on that pin, but it works fine as is.
  
  pinMode(1, OUTPUT);
  DmxSimple.usePin(1);

  // This sets the number of DMX channels to 3*number of lights. It might
  // actually need to be +1, my seventh light's blue channel wasn't active.
  // I was using very old used lights though, and didn't test whether
  // making this 25 instead of 24 would cause that channel to work.
  
  DmxSimple.maxChannel(24);

  // Configure the ADM2582E to transmit only (no DMX receive in this code).
  // These correspond to the Receive and Transmit enable lines on the specific
  // chip and so are not included in the DmxSimple library tutorial. They
  // are not necessary if you are using another way to send DMX signals.
  
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH);
  pinMode(19, OUTPUT);
  digitalWrite(19, HIGH);

  // Configure input ADC for 13-bit mode, listed as the noise floor for the
  // Teensy 3.1 ADC. It would be fine to use 16 bits but I didn't see much
  // point between the noise in the ADC and the noise intrinsic in the audio
  // signal.
  
  analogReadResolution(13);

  // Set the audio zero level to ~4096 to start since it should be roughly
  // mid-scale, and sets start values for the threshold, max volume seen,
  // and light decay period. Once you get some info on what your board signal
  // levels look like this can be shifted to achieve a faster time to
  // when the board is working well.
  
  audioZero = audioZeroStart;
  audioRMSMax = audioRMSMaxStart;
  dThreshold = dThresholdStart;
  lightDecayPeriod = lightDecayPeriodStart;

  // Set the beat detected flag to false to start.
  
  beatdetected = false;

  // Starting parameters for filtered signals.
  
  daudioRMSFiltered = 0;
  beatCountsFiltered = 0;
  audioRMSFiltered = 0;

  // Initialize the peak beat volume storage.
  
  peakBeatVolume = 0;
}

void loop() {

  // This portion handles audio sampling and analysis.
  
  if (audioSamplingTimer > 22) { // No, really, 22us is roughly 44.1kHz! Coincidence?
    audioSamplingTimer = audioSamplingTimer - 22;
    
    // Input capacitively coupled, mid-scale centered audio signal.
    
    float audioSignal = analogRead(0);
  
    // Use exponential smoothing to capture the DC offset. This is using
    // the pure audio signal which should always have a stable DC offset
    // due to being capacitively coupled to the analog input.
    
    audioZero = 0.999999*audioZero + 0.000001*audioSignal;
  
    // Use the audioZero to calculate the RMS audio signal. This gives
    // the volume of the audio rather than an AC signal that is hard
    // to interpret. In past versions, abs() was used instead of RMS with
    // little loss of performance and faster calculation for use on
    // lower end hardware.
    
    float audioRMS = sqrt(pow(audioSignal - audioZero, 2));
    float oldaudioRMSFiltered = audioRMSFiltered;
    audioRMSFiltered = 0.99*audioRMSFiltered + 0.01*audioRMS;
  
    // If the audioRMSFiltered value exceeds the previously seen maximum,
    // set the audioRMSMax to the new value.
    
    if (audioRMSFiltered > audioRMSMax) audioRMSMax = audioRMSFiltered;
  
    // but every loop through, drag the audioRMSMax back towards zero so
    // that it doesn't only ever grow larger when it sees a volume increase.
    
    audioRMSMax = 0.99999*audioRMSMax + 0.00001*audioRMSMaxStart;

    // Next we need the rough derivitive of the piece, so we take the
    // current audioRMSFiltered value and subtract the immediately prior
    // sample. Very light exponential smoothing to avoid false pops.

    daudioRMSFiltered = 0.99*daudioRMSFiltered + 0.01*(audioRMSFiltered - oldaudioRMSFiltered);

    // and if the daudioRMSFIltered is higher than a threshold, indicate
    // beat detection.

    if (daudioRMSFiltered > dThreshold) {
      beatdetected = true;

      // The derivitive might exceed the threshold for several samples, so only
      // save the peak volume over the beat to set the light brightness peak during
      // that flash.
      
      peakBeatVolume = min(max(audioRMSFiltered/audioRMSMax, peakBeatVolume), 1);

      // Uncomment this to get debug values for tuning the audio signal chain.
      // However, it executes every audio sample, so disable during normal operation
      // to avoid taxing the processor too much.
      
      // Serial.println("Beat Detected with dAudio: " + String(daudioRMSFiltered) + " and amplitude " + String(peakBeatVolume));
    }
  }

  // This portion handles the actual DMX light updates.
  
  if (sendtimer >= timestep) {
    sendtimer = sendtimer - timestep;

    for (unsigned int i=0; i<8; i++) {
      // Rotate hue of all lights based on parameters at the start of the program.
      huearray[i] = fmod(huearray[i] + ((float)timestep/1000)*hueStepPerSecond, 360);
    }

    // Uncomment the below to get a stream of the detected audio DC offset level
    // useful for debugging, as well as the realtime audio RMS signal and recorded
    // maximum RMS value it is normalizing against.
    
    // Serial.println("Audio Zero: " + String(audioZero) + " Audio RMS: " + String(audioRMSFiltered) + " Normalized to Max of: " + String(audioRMSMax));

    // This section handles the case where a beat was detected.
    
    if (beatdetected) {
      beatdetected = false;
      
      // Increment the beatCounts by one, but only do this every DMX timestep to do
      // de-bouncing (i.e. don't count beats detected closer than 10ms apart as distinct).
      
      beatCounts++;

      // Set the light brightnesses to the normalized volume immediately.
      
      for (unsigned int i=0; i<8; i++) {
        intensityarray[i] = max(peakBeatVolume, intensityarray[i]);
      }

      // And reset the peak beat volume for the next beat detection event.
      peakBeatVolume = 0;
    }

    // But regardless, always be letting the light intensity drop down to the min level.
    // The decay speed varies based on song genre and the constants above.
    
    float timeConstant = min(timestep/lightDecayPeriod, 0.5);
    for (unsigned int i=0; i<8; i++) {
      intensityarray[i] = (1-timeConstant)*intensityarray[i] + timeConstant*intensityMin;
    }

    // And writecolors actually uses the DmxSimple library and HSI2RGB function
    // to send updated values out to the lights.
    
    writecolors();
  }

  // Finally, this does some automatic fudging of the beat detection threshold
  // and the time decay constant for the light to return to normal after a beat
  // to help compensate for quiet/classical/ambient music versus electronica.
  // The general idea is that if the threshold is pushed high because the song
  // has lots of light beats that would be too confusing to see if they were
  // all detected, the length of time of a light pulse up and back down is shorter
  // since the song overall sounds much more rhythmic and drummy.

  if (beatTimer > 1000) {
    beatTimer = beatTimer - 1000;
    beatCountsFiltered = max(0.1, 0.7*beatCountsFiltered + 0.3*(float)beatCounts);
    
    dThreshold = dThreshold + 0.05*(beatCountsFiltered - bpsTarget);
    dThreshold = max(dThreshold, dThresholdMin);
    
    lightDecayPeriod = max((dThresholdMin/dThreshold)*300, 30);

    // Debug information about the beat detection tuning, which is infrequent enough
    // that I left it uncommented here.
    
    Serial.println("Detected " + String(beatCountsFiltered) + " BPS, target is " + String(bpsTarget) + ". New threshold is " + String(dThreshold) + ". Pulse decay TC is " + String(lightDecayPeriod));
    beatCounts = 0;
  }
}

void writecolors() {
  for (unsigned int i=0; i<8; i++) {    
    // Convert hue to RGB value assuming fully saturated and full brightness.
    int rgb[3];
    hsi2rgb(huearray[i], 1, intensityarray[i], rgb);

    // Write them out to the DMX port.
    DmxSimple.write(i*3, rgb[0]);
    DmxSimple.write(i*3+1, rgb[1]);
    DmxSimple.write(i*3+2, rgb[2]);
  }
}

void hsi2rgb(float H, float S, float I, int* rgb) {
  int r, g, b;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
    
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }
  rgb[0]=r;
  rgb[1]=g;
  rgb[2]=b;
}
