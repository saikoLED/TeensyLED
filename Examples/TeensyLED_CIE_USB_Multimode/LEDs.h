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
#include <memory>

class CIELED {
  private:
    float _u, _v, _maxvalue;
    int _pin;
  public:
    CIELED(float u, float v, float maxvalue, int pin);
    CIELED(void);
    float getU(void);
    float getV(void);
    float getMax(void);
    int getPin(void);
};

class HSIColor {
  private:
    float _hue, _saturation, _intensity;
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
};

class Colorspace {
  private:
    std::vector<CIELED> _LEDs;
    CIELED _white;
    std::vector<float> _slope;
    std::vector<float> _angle;
  public:
    Colorspace(CIELED &white);
    Colorspace(void);
    void addLED(CIELED &LED);
    float getAngle(int LEDnum);
    float getSlope(int LEDnum);
    std::vector<float> Hue2LEDs(HSIColor &HSI);
    std::vector<int> getPins(void);
    std::vector<float> getMaxValues(void);
};

class HSIFader {
  private:
    HSIColor _colors[2];
    unsigned long _startmicros;
    unsigned long _delaymicros;
    int _direction;
  public:
    HSIFader(HSIColor color1, HSIColor color2, float time, int direction);
    HSIColor getHSIColor();
    void setFader(HSIColor color1, HSIColor color2, float time, int direction);
    boolean isRunning(void);
};

class RandomFader {
  private:
    std::vector<CIELED> _LEDs;
    std::vector<CIELED> _effectLEDs;
    unsigned long _startmicros;
    unsigned long _periodmicros;
    unsigned int _LED1, _LED2;
    std::vector<float> _effectprob;
    std::vector<unsigned int> _effect;
  public:
    RandomFader(float period);
    void startRandom(float period);
    void addLED(CIELED LED);
    void addEffectLED(CIELED LED, float effectprob);
    std::vector<float> getLEDs(void);
    std::vector<int> getPins(void);
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
    void setColor(int num, HSIColor color);
};

class HSICycler {
  private:
    HSIColor _color;
    unsigned long _lastmicros;
    float _huestep;
  public:
    HSICycler(HSIColor color, float time, int dir);
    HSIColor getHSIColor();
    void setCycler(HSIColor color, float time, int dir);
};

class RGBWLamp {
  private:
    int _resolution;
    std::vector<int> _pins;
    std::vector<float> _maxvalues;
    std::shared_ptr<Colorspace> _colorspace;
    float _PWMfrequency;
  public:
    RGBWLamp(int resolution, float PWMfrequency);
    void addColorspace(std::shared_ptr<Colorspace> colorspace);
    void setColor(HSIColor &color);
    void setLEDs(std::vector<float> &LEDs, std::vector<int> &pins);
    void begin(void);
};
