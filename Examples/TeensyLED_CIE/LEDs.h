//*********************************************************
//
// TeensyLED Controller Library
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

#pragma once

#include <Arduino.h>

class RGBWLamp {
  private:
    char _redpin, _greenpin, _bluepin, _whitepin, _resolution;
    float _hue, _saturation, _intensity;
    float _PWMfrequency;
  public:
    RGBWLamp(char redpin, char greenpin, char bluepin, char whitepin, char resolution, float PWMfrequency);
    void begin(void);
    void setHue(float hue) {
      _hue = fmod(hue, 360);
    };
    void setSaturation(float S) {
      _saturation = S>0?(S<1?S:1):0;
    };
    void setIntensity(float I) {
      _intensity = I>0?(I<1?I:1):0;
    }
    void setColor(void);
    float getHue(void) {return _hue;};
    float getSaturation(void) {return _saturation;};
    float getIntensity(void) {return _intensity;};
};


