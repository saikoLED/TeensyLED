//**************************************************************************
//
// TeensyLED CIE Controller
// Copyright Brian Neltner 2015
// Version 0.3 - May 29, 2015
//
// This provides a state of the art library and some basic example code for
// using the TeensyLED Controller board to generate RGBW colors in full
// CIE LCH colorspace. This example sets the hue between 0 and 360 and rotates
// around the color wheel. This angle is intersected with the line drawn
// between the nearest two LEDs with their CIE colorspace u' and v' coefficients
// to calculate the correct weighting of the two arbitrary CIE defined light
// sources to get the target CIE LCH hue.
//
// Note that in this version, for simplicity I have put in a bias to the red
// angle so that the red LED is at 0 degrees. In actuality, the red LED at
// 623 nm has a CIE hue of 9.67 degrees, but I think this will be easier
// to understand since it more closely matches the likely user goals.
//
// The data to calculate the CIE values for each LED was calculated by
// digitizing the power spectra provided for the LZ7 series LED from LEDEngin
// so you will need to get your own if you use a different LED (or just don't
// worry about it). Color correction to account for different LED brightness
// is done using the lumen output specified in the datasheet.
//
// TeensyLED Controller is free software: you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TeensyLED Controller is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
//
//***************************************************************************

#include "LEDs.h"
#include <DmxReceiver.h>

#define propgain 0.001

// Redpin, Greenpin, Bluepin, Whitepin, Resolution, Frequency.
RGBWLamp lamp(6, 22, 23, 9, 16, 183.106);

// Define DMX Device.
DmxReceiver dmx;
IntervalTimer dmxTimer;

// Global target hue and saturation for follower.
float targethue;
float targetsaturation;

void setup() {
  Serial.begin(115200);
  
  lamp.begin();
  dmx.begin();
  dmxTimer.begin(dmxTimerISR, 1000);
  
  targethue = 0;
  targetsaturation = 1.0;
  
  // Initialize light at the target hue, no brightness, fully saturated.
  lamp.setHue(targethue);
  lamp.setIntensity(0);
  lamp.setSaturation(targetsaturation);
  lamp.setColor();
  
  // Startup Routine.
  while (lamp.getIntensity() < 1) {
    lamp.setIntensity(lamp.getIntensity()+0.001);
//    updatetargethue();
//    updatetargetsaturation();
//    lamp.setHue(lamp.getHue() + propgain*(targethue - lamp.getHue()));
//    lamp.setSaturation(lamp.getSaturation() + propgain*(targetsaturation - lamp.getSaturation()));    
    lamp.setColor();
    delay(5);
  }
}

elapsedMillis elapsed;
boolean updated = false;

void loop() {
//  if (dmx.newFrame()) {
//    Serial.println("New DMX Frame.");
//  }

  if (Serial.available()) {
    // Read until there is a newline.
    String received = Serial.readStringUntil(0x0D);
    // Check that the value is a valid float.
    if (checkFloat(received) == 0) {
      // Setting hue automatically rotates into 0-360.
      float hue = received.toFloat();
      lamp.setHue(hue);
      Serial.println("Setting hue to " + String(hue) + " degrees.");
      updated = true;
    }
    else Serial.println("Error parsing hue.");
  }
  
  if (elapsed > 5) {
//    updatetargethue();
//    updatetargetsaturation();
//    lamp.setHue(lamp.getHue() + propgain*(targethue - lamp.getHue()));
//    lamp.setSaturation(lamp.getSaturation() + propgain*(targetsaturation - lamp.getSaturation()));
    if (updated) {
      lamp.setColor();
      updated = false;
    }
    elapsed -= 5;
  }
}

void updatetargethue() {
  targethue += (rand()%360-180)*0.01;
  targethue = fmod(targethue+360, 360);
}

void updatetargetsaturation() {
  targetsaturation += (rand()%10000-5000)*0.00001;
  targetsaturation = targetsaturation>0.5?(targetsaturation<1?targetsaturation:1):0.5;
}

void dmxTimerISR(void) {
  dmx.bufferService();
}

int checkFloat(String data) {
  // The simplest way to go about testing the string for only numerical inputs
  // since Arduino doesn't implement regex, is probably to count the number of 
  // characters and then count the number of instances of each valid number
  // character. i.e. [0-9] and the decimal point for a float after trimming
  // whitespace or newlines.
  
  data.trim();
  // Check to make sure the string isn't now blank.
  if (data.length() == 0) return -2;
  
  unsigned int runningtotal = 0;
  
  for (unsigned int j=0; j<data.length(); j++) {
    char activechar = data.charAt(j);
    // Test against valid cahracter list.
    if ((activechar == '-') ||
        (activechar == '.') ||
        (activechar == '0') ||
        (activechar == '1') ||
        (activechar == '2') ||
        (activechar == '3') ||
        (activechar == '4') ||
        (activechar == '5') ||
        (activechar == '6') ||
        (activechar == '7') ||
        (activechar == '8') ||
        (activechar == '9')) runningtotal++;
  }
  
  if (runningtotal == data.length()) return 0;
  else return -1;
}
