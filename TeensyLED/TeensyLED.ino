//*********************************************************
//
// TeensyLED Controller
// Copyright Brian Neltner 2015
// Version 0.1 - April 13, 2015
//
// This file is part of TeensyLED Controller.
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
//**********************************************************

#include "TeensyLED.h"

#define propgain 0.001

// Redpin, Greenpin, Bluepin, Whitepin, Resolution, Frequency.
RGBWLamp lamp(3, 4, 5, 6, 16, 732);

// Global target hue and saturation for follower.
float targethue;
float targetsaturation;

void setup() {
  Serial.begin(9600);
  
  targethue = 0;
  targetsaturation = 0.75;
  
  // Initialize light at the target hue, no brightness, fully saturated.
  lamp.setHue(targethue);
  lamp.setIntensity(0);
  lamp.setSaturation(targetsaturation);
  lamp.setColor();
  
  // Startup Routine.
  while (lamp.getIntensity() < 1) {
    lamp.setIntensity(lamp.getIntensity()+0.001);
    updatetargethue();
    updatetargetsaturation();
    lamp.setHue(lamp.getHue() + propgain*(targethue - lamp.getHue()));
    lamp.setSaturation(lamp.getSaturation() + propgain*(targetsaturation - lamp.getSaturation()));
    lamp.setColor();
    delay(1);
  }
}

void loop() {
  updatetargethue();
  updatetargetsaturation();
  lamp.setHue(lamp.getHue() + propgain*(targethue - lamp.getHue()));
  lamp.setSaturation(lamp.getSaturation() + propgain*(targetsaturation - lamp.getSaturation()));
  lamp.setColor();
  delay(1);
}

void updatetargethue() {
  targethue += (rand()%360-180)*0.01;
  targethue = fmod(targethue+360, 360);
}

void updatetargetsaturation() {
  targetsaturation += (rand()%10000-5000)*0.00001;
  targetsaturation = targetsaturation>0.5?(targetsaturation<1?targetsaturation:1):0.5;
}
