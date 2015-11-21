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
#include <vector>

class CIELED {
  private:
    float _u, _v, _maxvalue;
    byte _pin;
  public:
    CIELED(float u, float v, float maxvalue, byte pin);
    float getU(void);
    float getV(void);
    float getMax(void);
    byte getPin(void);
};

class colorspace {
  private:
    std::vector<CIELED> _LEDs;
    CIELED _white;
    std::vector<float> _slope;
  public:
    colorspace(CIELED white);
    void begin(void);
    void addLED(CIELED LED);
};

class HSIColor {
  private:
    float _hue, _saturation, _intensity;
    boolean _updated;
  public:
    HSIColor(float hue, float saturation, float intensity);
    HSIColor(void);
    void setHue(float hue);
    void setSaturation(float saturation);
    void setIntensity(float intensity);
    void setHSI(float hue, float saturation, float intensity);
    float getHue(void);
    float getSaturation(void);
    float getIntensity(void);
    void getHSI(float *HSI);
    void getRGBW(float *RGBW);
    boolean isupdated();
};

class HSIFader {
  private:
    HSIColor _colors[2];
    unsigned long _startmicros;
    unsigned long _delaymicros;
    byte _direction;
  public:
    HSIFader(HSIColor color1, HSIColor color2, float time, byte direction);
    HSIColor getHSIColor();
    void setFader(HSIColor color1, HSIColor color2, float time, byte direction);
    boolean isRunning(void);
};

class HSIStrober {
  private:
    HSIColor _colors[2];
    unsigned long _startmicros;
    unsigned long _periodmicros;
    unsigned long _periodmicrosDB;
  public:
    HSIStrober(HSIColor color1, HSIColor color2, float time);
    HSIColor getHSIColor();
    void setStrober(HSIColor color1, HSIColor color2, float time);
    void setPeriod(float time);
    void setColor(byte num, HSIColor color);
};

class RGBWLamp {
  private:
    char _resolution;
    char _pinnum[4];
    float _maxvalue[4];
    float _PWMfrequency;
  public:
    RGBWLamp(char redpin, float redmax, char greenpin, float greenmax, char bluepin, float bluemax, char whitepin, float whitemax, char resolution, float PWMfrequency);
    void begin(void);
    void setColor(HSIColor &color);
};
