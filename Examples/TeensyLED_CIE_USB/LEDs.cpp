#include "LEDs.h"

// These are the constants calculated from the specific hardware for CIE LUV colorspace.
// u* and v* are not scaled by luminosity because it's irrelevant to angle calculations.

#define Red_ustar 0.311170122
#define Red_vstar 0.0530073442
#define Green_ustar -0.1429469589
#define Green_vstar 0.1050521141
#define Blue_ustar -0.0277372709
#define Blue_vstar -0.3581532103
#define White_uprime 0.2025316456
#define White_vprime 0.4699367089

// Slopes between different locations in colorspace.
#define RGm -0.1146065014
#define GBm -4.0205414357
#define BRm 1.21319441

// Define the angles each color is nominally at.
#define RedAngle 9.6674474725
#define GreenAngle 143.6877311667
#define BlueAngle 265.5715525645

// Brightness Scaling
#define RedMax 78/78
#define GreenMax 125/125
#define BlueMax 30/30
#define WhiteMax 180/180

//#define M_PI 3.14159265358979323846264338327950288

RGBWLamp::RGBWLamp(char redpin, float redmax, char greenpin, float greenmax, char bluepin, float bluemax, char whitepin, float whitemax, char resolution, float PWMfrequency) :
  _resolution(resolution),
  _PWMfrequency(PWMfrequency) {
    _pinnum[0] = redpin;
    _pinnum[1] = greenpin;
    _pinnum[2] = bluepin;
    _pinnum[3] = whitepin;
    _maxvalue[0] = redmax;
    _maxvalue[1] = greenmax;
    _maxvalue[2] = bluemax;
    _maxvalue[3] = whitemax;
  }

void RGBWLamp::begin(void) {
  for (int i=0; i<4; i++) {
    pinMode(_pinnum[i], OUTPUT);
    analogWriteFrequency(_pinnum[i], _PWMfrequency);
  }
  analogWriteResolution(_resolution);
}

void RGBWLamp::setColor(HSIColor &color) {
  float rgbw[4];
  // Get the RGBW version.
  color.getRGBW(rgbw);
  for (int i=0; i<4; i++) {
    analogWrite(_pinnum[i], ((1<<_resolution)-1) * rgbw[i] * _maxvalue[i]);
  }
}

HSIColor::HSIColor(float hue, float saturation, float intensity) {
  setHue(hue);
  setSaturation(saturation);
  setIntensity(intensity);
}

// Default constructor.
HSIColor::HSIColor(void) {
  setHue(0);
  setSaturation(0);
  setIntensity(0);
}

void HSIColor::setHue(float hue) {
  _hue = fmod(hue, 360);
  _updated = true;
}

void HSIColor::setSaturation(float saturation) {
  _saturation = saturation>0?(saturation<1?saturation:1):0;
  _updated = true;
}

void HSIColor::setIntensity(float intensity) {
  _intensity = intensity>0?(intensity<1?intensity:1):0;
  _updated = true;
}

void HSIColor::setHSI(float hue, float saturation, float intensity) {
  setHue(hue);
  setSaturation(saturation);
  setIntensity(intensity);
}

float HSIColor::getHue(void) {
  return _hue;
}

float HSIColor::getSaturation(void) {
  return _saturation;
}

float HSIColor::getIntensity(void) {
  return _intensity;
}

void HSIColor::getHSI(float *HSI) {
  HSI[0] = getHue();
  HSI[1] = getSaturation();
  HSI[2] = getIntensity();
}

// And this is the meat. Converts the abstract color into RGBW (scaled 0-1).
void HSIColor::getRGBW(float *RGBW) {
  float H = fmod(_hue+RedAngle+360,360);
  float S = _saturation;
  float I = _intensity;
  
  float tanH = tan(M_PI*fmod(H,360)/(float)180); // Get the tangent since we will use it often.
  float ustar = 0;
  float vstar = 0;
  
  // Check the range to determine which intersection to do.
  
  if ((H >= RedAngle) && (H < GreenAngle)) {
    // Then we are finding the point between Red and Green with the right hue.
    ustar=(Green_vstar-RGm*Green_ustar)/(tanH - RGm);
    vstar=tanH/(RGm-tanH) *(RGm*Green_ustar - Green_vstar);
    RGBW[0] = I * S * abs(ustar-Green_ustar)/abs(Green_ustar - Red_ustar);
    RGBW[1] = I * S * abs(ustar-Red_ustar)/abs(Green_ustar - Red_ustar);
    RGBW[2] = 0;
    RGBW[3] = I*(1-S);
  }
  else if ((H >= GreenAngle) && (H < BlueAngle)) {
    // Then we are finding the point between Green and Blue with the right hue.
    ustar=(Blue_vstar-GBm*Blue_ustar)/(tanH - GBm);
    vstar=tanH/(GBm-tanH) *(GBm*Blue_ustar - Blue_vstar);
    RGBW[0] = 0;
    RGBW[1] = I * S * abs(ustar-Blue_ustar)/abs(Green_ustar - Blue_ustar);
    RGBW[2] = I * S * abs(ustar-Green_ustar)/abs(Green_ustar - Blue_ustar);
    RGBW[3] = I*(1-S);
  }
  else if (((H >= BlueAngle) && (H < 360)) || (H < RedAngle)) {
    // Then we are finding the point between Green and Blue with the right hue.
    ustar=(Red_vstar-BRm*Red_ustar)/(tanH - BRm);
    vstar=tanH/(BRm-tanH) *(BRm*Red_ustar - Red_vstar);
    RGBW[0] = I * S * abs(ustar-Blue_ustar)/abs(Red_ustar - Blue_ustar);
    RGBW[1] = 0;
    RGBW[2] = I * S * abs(ustar-Red_ustar)/abs(Red_ustar - Blue_ustar);
    RGBW[3] = I*(1-S);
  }
  
  Serial.println("CIE Hue: " + String(H,3) + ", (u', v'): (" + String(ustar+White_uprime,3) + ", " + String(vstar+White_vprime,3) + "), RGBW: " + String((100*RGBW[0]),1) + "% " + String((100*RGBW[1]),1) + "% " + String((100*RGBW[2]),1) + "% " + String((100*RGBW[3]),1) + "%");
  _updated = false;
}

boolean HSIColor::isupdated(void) {
  return _updated;
}  

HSIFader::HSIFader(HSIColor color1, HSIColor color2, float time, byte direction) {
  setFader(color1, color2, time, direction);
}

void HSIFader::setFader(HSIColor color1, HSIColor color2, float time, byte direction) {
  _colors[0] = color1;
  _colors[1] = color2;
  _delaymicros = time*1000;
  _startmicros = micros();
  _direction = direction;
  // If the hues match, set to constant hue.
  if (_colors[0].getHue() == _colors[1].getHue()) _direction = 2;
}

HSIColor HSIFader::getHSIColor() {
  long time = micros() - _startmicros;
  float hue;
  // If direction is 1, rotate positive.
  if (_direction == 1) hue = (_colors[0].getHue() * (1-time/_delaymicros) + _colors[1].getHue()*time/_delaymicros);
  // If direction is 0, rotate negative.
  else if (_direction == 0) hue = (_colors[0].getHue() * (1-time/_delaymicros) + (_colors[1].getHue()-360)*time/_delaymicros);
  // If direction is -1, constant hue.
  else if (_direction == 2) hue = _colors[0].getHue();
  // Otherwise, somethign weird happened. Just set to red.
  else hue = 0;
  
  float saturation = (_colors[0].getSaturation() * (1-time/_delaymicros) + _colors[1].getSaturation()*time/_delaymicros);
  float intensity = (_colors[0].getIntensity() * (1-time/_delaymicros) + _colors[1].getIntensity()*time/_delaymicros);
  return HSIColor(hue, saturation, intensity);
}

boolean HSIFader::isRunning(void) {
  long time = micros() - _startmicros;
  if (time <= _delaymicros) return true;
  else return false;
}

HSIStrober::HSIStrober(HSIColor color1, HSIColor color2, float time) {
  setStrober(color1, color2, time);
  _startmicros = micros();
  _periodmicros = _periodmicrosDB;
}

void HSIStrober::setStrober(HSIColor color1, HSIColor color2, float time) {
  _colors[0] = color1;
  _colors[1] = color2;
  _periodmicrosDB = time*1000;
}

void HSIStrober::setPeriod(float time) {
  _periodmicrosDB = time*1000;
}

void HSIStrober::setColor(byte num, HSIColor color) {
  _colors[num] = color;
}

HSIColor HSIStrober::getHSIColor(void) {
  long time = micros() - _startmicros;
  // For first half, show color1.
  if (time < _periodmicros/2) {
    return _colors[0];
  }
  if (time > _periodmicros/2) {
    if (time > _periodmicros) {
      // Load the double buffered period.
      _periodmicros = _periodmicrosDB;
      _startmicros = micros();
      return _colors[0];
    }
    else return _colors[1];
  }
  // Shouldn't get here.
  return _colors[0];
}

CIELED::CIELED(float u, float v, float maxvalue, byte pin) :
  _u(u),
  _v(v),
  _maxvalue(maxvalue),
  _pin(pin) {
}
  
float CIELED::getU(void) {
  return _u;
}

float CIELED::getV(void) {
  return _v;
}

float CIELED::getMax(void) {
  return _maxvalue;
}

byte CIELED::getPin(void) {
  return _pin;
}
