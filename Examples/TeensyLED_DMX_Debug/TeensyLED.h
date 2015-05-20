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

RGBWLamp::RGBWLamp(char redpin, char greenpin, char bluepin, char whitepin, char resolution, float PWMfrequency) :
  _redpin(redpin),
  _greenpin(greenpin),
  _bluepin(bluepin),
  _whitepin(whitepin),
  _resolution(resolution),
  _hue(0),
  _saturation(1),
  _intensity(0),
  _PWMfrequency(PWMfrequency) {}

void RGBWLamp::begin(void) {
  pinMode(_redpin, OUTPUT);
  pinMode(_greenpin, OUTPUT);
  pinMode(_bluepin, OUTPUT);
  pinMode(_whitepin, OUTPUT);
  analogWriteFrequency(_redpin, _PWMfrequency);
  analogWriteFrequency(_greenpin, _PWMfrequency);
  analogWriteFrequency(_bluepin, _PWMfrequency);
  analogWriteFrequency(_whitepin, _PWMfrequency);
  analogWriteResolution(_resolution);
}

void RGBWLamp::setColor(void) {
  float r, g, b, w;
  float H = _hue;
  float S = _saturation;
  float I = _intensity;
  float cos_h, cos_1047_h;
  H = 3.14159*H/(float)180; // Convert to radians.
  
  // This section is modified by the addition of white so that it assumes 
  // fully saturated colors, and then scales with white to lower saturation.
  //
  // Next, scale appropriately the pure color by mixing with the white channel.
  // Saturation is defined as "the ratio of colorfulness to brightness" so we will
  // do this by a simple ratio wherein the color values are scaled down by (1-S)
  // while the white LED is placed at S.
  
  // This will maintain constant brightness because in HSI, R+B+G = I. Thus, 
  // S*(R+B+G) = S*I. If we add to this (1-S)*I, where I is the total intensity,
  // the sum intensity stays constant while the ratio of colorfulness to brightness
  // goes down by S linearly relative to total Intensity, which is constant.

  if(H < 2.09439) {
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    r = S*I/3*(1+cos_h/cos_1047_h);
    g = S*I/3*(1+(1-cos_h/cos_1047_h));
    b = 0;
    w = (1-S)*I;
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    g = S*I/3*(1+cos_h/cos_1047_h);
    b = S*I/3*(1+(1-cos_h/cos_1047_h));
    r = 0;
    w = (1-S)*I;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    b = S*I/3*(1+cos_h/cos_1047_h);
    r = S*I/3*(1+(1-cos_h/cos_1047_h));
    g = 0;
    w = (1-S)*I;
  }
  
  // Mapping Function from rgbw = [0:1] onto their respective ranges.
  // For standard use, this would be [0:1]->[0:0xFFFF] for instance.

  // Here instead I am going to try a parabolic map followed by scaling.  
  analogWrite(_redpin, ((1<<_resolution)-1)*r);
  analogWrite(_greenpin, ((1<<_resolution)-1)*g);
  analogWrite(_bluepin, ((1<<_resolution)-1)*b);
  analogWrite(_whitepin, ((1<<_resolution)-1)*w);
}
