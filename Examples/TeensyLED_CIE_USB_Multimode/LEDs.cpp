#include "LEDs.h"

RGBWLamp::RGBWLamp(int resolution, float PWMfrequency) :
  _resolution(resolution),
  _PWMfrequency(PWMfrequency) {
}

void RGBWLamp::begin(void) {
  for (std::vector<int>::iterator i=_pins.begin(); i != _pins.end(); ++i) {
    pinMode(*i, OUTPUT);
    analogWrite(*i, 0);
    analogWriteFrequency(*i, _PWMfrequency);
  }
  analogWriteResolution(_resolution);
}

void RGBWLamp::setColor(HSIColor &color) {
  std::vector<float> LEDs = _colorspace->Hue2LEDs(color);
  for (int i=0; i<LEDs.size(); i++) {
    LEDs[i] = _maxvalues[i] * LEDs[i];
  }
  setLEDs(LEDs, _pins);
}

void RGBWLamp::setLEDs(std::vector<float> &LEDs, std::vector<int> &pins) {
  for (int i=0; i<LEDs.size(); i++) {
    analogWrite(pins[i], 0xFFFF * LEDs[i]);
//    Serial.print(LEDs[i]);
//    Serial.print(" ");
  }
//  Serial.println("");
}

void RGBWLamp::addColorspace(std::shared_ptr<Colorspace> colorspace) {  
  _pins = colorspace->getPins();
  _maxvalues = colorspace->getMaxValues();
  _colorspace = colorspace;
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
}

void HSIColor::setSaturation(float saturation) {
  _saturation = saturation>0?(saturation<1?saturation:1):0;
}

void HSIColor::setIntensity(float intensity) {
  _intensity = intensity>0?(intensity<1?intensity:1):0;
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

HSIFader::HSIFader(HSIColor color1, HSIColor color2, float time, int direction) {
  setFader(color1, color2, time, direction);
}

void HSIFader::setFader(HSIColor color1, HSIColor color2, float time, int direction) {
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
  // If direction is 2, constant hue.
  else if (_direction == 2) hue = _colors[0].getHue();
  // Otherwise, somethign weird happened. Just set to red.
  else hue = 0;
  
  float saturation = (_colors[0].getSaturation() * (1-(float)time/_delaymicros) + _colors[1].getSaturation()*(float)time/_delaymicros);
  float intensity = (_colors[0].getIntensity() * (1-(float)time/_delaymicros) + _colors[1].getIntensity()*(float)time/_delaymicros);
  return HSIColor(hue, saturation, intensity);
}

boolean HSIFader::isRunning(void) {
  long time = micros() - _startmicros;
  if (time <= _delaymicros) return true;
  else return false;
}

RandomFader::RandomFader(float period) {
  _periodmicros = period*1000;
}

void RandomFader::startRandom(float period) {
  _periodmicros = period*1000;
  _LED1 = random(_LEDs.size());
  _LED2 = random(_LEDs.size());
  while(_LED1 == _LED2) _LED2 = random(_LEDs.size());
  _startmicros = micros();
}

void RandomFader::addLED(CIELED LED) {
  _LEDs.push_back(LED);
}

std::vector<float> RandomFader::getLEDs(void) {
  std::vector<float> LEDOutputs;
  for (int i=0; i<(_LEDs.size()+_effectLEDs.size()); i++) {
    LEDOutputs.push_back(0);
  }
  long time = micros() - _startmicros;
  if (time > _periodmicros) {
    _LED1 = _LED2;
    _LED2 = random(_LEDs.size());
    while(_LED1 == _LED2) _LED2 = random(_LEDs.size());
    
    // Handle effect LED state.
    if (_effectLEDs.size() != 0) {
      for (unsigned int i=0; i<_effectLEDs.size(); i++) {
        float diceroll = (float)random(1000)/1000;
        switch (_effect[i]) { // Change state based on current state.
          case 0: // Currently off.
            if (diceroll < _effectprob[i]) _effect[i] = 2;
            else _effect[i] = 0;
            break;
          case 1: // Currently on.
            if (diceroll < _effectprob[i]) _effect[i] = 1;
            else _effect[i] = 3;
            break;
          case 2: // Fading on.
            if (diceroll < _effectprob[i]) _effect[i] = 1;
            else _effect[i] = 3;
            break;
          case 3: // Fading off.
            if (diceroll < _effectprob[i]) _effect[i] = 2;
            else _effect[i] = 0;
            break;
        }
      }
    }
            
    _startmicros += _periodmicros;
  }
  LEDOutputs[_LED1] = _LEDs[_LED1].getMax()*(1-((float)time/_periodmicros));
  LEDOutputs[_LED2] = _LEDs[_LED2].getMax()*((float)time/_periodmicros);
  
  if (_effectLEDs.size() != 0) {
    for (unsigned int i=0; i<_effectLEDs.size(); i++) {
      switch (_effect[i]) {
        case 0: // Case where it is off.
          LEDOutputs[i+_LEDs.size()] = 0;
          break;
        case 1: // Case where it is on.
          LEDOutputs[i+_LEDs.size()] = 1;
          break;
        case 2: // Case where it is turning on.
          LEDOutputs[i+_LEDs.size()] = (float)time/_periodmicros;
          break;
        case 3: // Case where it is turning off.
          LEDOutputs[i+_LEDs.size()] = 1-(float)time/_periodmicros;
          break;
      }
    }
  }
  
  // For debugging, print the actual output values.
//  Serial.println("Output Values. LED1 is " + String(_LED1) + " LED2 is " + String(_LED2));
//  for (std::vector<float>::iterator i=LEDOutputs.begin(); i != LEDOutputs.end(); ++i) {
//    Serial.print(*i, 2);
//    Serial.print(" ");
//  }
//  Serial.println("");
  return LEDOutputs;
}

std::vector<int> RandomFader::getPins(void) {
  std::vector<int> pins;
  for (int i=0; i<_LEDs.size(); i++) {
    pins.push_back(_LEDs[i].getPin());
  }
  if (_effectLEDs.size() != 0) {
    for (int i=0; i<_effectLEDs.size(); i++) {
      pins.push_back(_effectLEDs[i].getPin());
    }
  }
  return pins;
}

void RandomFader::addEffectLED(CIELED LED, float effectprob) {
  _effectLEDs.push_back(LED);
  _effectprob.push_back(effectprob);
  _effect.push_back(0);
}

HSICycler::HSICycler(HSIColor color, float time, int dir) :
  _color(color) {
  if (dir == 1) _huestep = 0.36/(float)time;
  if (dir == 0) _huestep = -0.36/(float)time;
  _lastmicros = micros();
}

void HSICycler::setCycler(HSIColor color, float time, int dir) {
  _color = color;
  if (dir == 1) _huestep = 0.36/time;
  if (dir == 0) _huestep = -0.36/time;
  _lastmicros = micros();
}

HSIColor HSICycler::getHSIColor(void) {
  long time = micros() - _lastmicros;
  _color.setHue(_color.getHue() + (time * _huestep));
  _lastmicros = micros();
  return _color;
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

void HSIStrober::setColor(int num, HSIColor color) {
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

CIELED::CIELED(float u, float v, float maxvalue, int pin) :
  _u(u),
  _v(v),
  _maxvalue(maxvalue),
  _pin(pin) {
}

CIELED::CIELED(void) {
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

int CIELED::getPin(void) {
  return _pin;
}

Colorspace::Colorspace(CIELED &white) :
  _white(white) {
}

Colorspace::Colorspace(void) {
}

void Colorspace::addLED(CIELED &LED) {
  // To figure out where to put it in the colorspace, calculate the angle from the white point.
  float uLED = LED.getU();
  float vLED = LED.getV();
  float uWHITE = _white.getU();
  float vWHITE = _white.getV();
  
  float angle = fmod((180/M_PI) * atan2((vLED - vWHITE),(uLED - uWHITE)) + 360, 360);
  
  // If it is the first LED, simply place it in the array.
  if (_LEDs.empty()) {
    _LEDs.push_back(LED);
    _angle.push_back(angle);
    // With only one LED, slope is undefined.
    _slope.push_back(0);
  }
  // Otherwise, place the LED at the appropriate point in the array, and also recalculate slopes.
  else {
    int insertlocation;
    // Iterate through until finding the first location where the angle fits.
    for (insertlocation = 0; (_angle[insertlocation] < angle) && (insertlocation < _angle.size()); insertlocation++);
    _LEDs.insert(_LEDs.begin() + insertlocation, LED);
    _angle.insert(_angle.begin() + insertlocation, angle);
    
    // Add an empty slope since we need to recalculate them all once they're ordered.
    _slope.push_back(0);
    
    // And then recalculate all slopes. Last slope is a special case.
    for (int i=0; i<(_LEDs.size()-1); i++) {
      _slope[i] = (_LEDs[i+1].getV() - _LEDs[i].getV()) / (_LEDs[i+1].getU() - _LEDs[i].getU());
    }
    _slope[_LEDs.size()-1] = (_LEDs[0].getV() - _LEDs[_LEDs.size()-1].getV()) / (_LEDs[0].getU() - _LEDs[_LEDs.size()-1].getU());
  } 
  
//  // For debugging, print the current array of angles.
//  Serial.println("Current LED Angles");
//  for (std::vector<float>::iterator i=_angle.begin(); i != _angle.end(); ++i) {
//    Serial.println(*i, 5);
//  }
//  Serial.println("");
//  
//  // For debugging, print the current array of slopes.
//  Serial.println("Current LED Slopes");
//  for (std::vector<float>::iterator i=_slope.begin(); i != _slope.end(); ++i) {
//    Serial.println(*i, 5);
//  }
//  Serial.println("");
}

float Colorspace::getAngle(int num) {
  return _angle[num];
}

std::vector<int> Colorspace::getPins(void) {
  std::vector<int> pins;
  for (int i=0; i<_LEDs.size(); i++) {
    pins.push_back(_LEDs[i].getPin());
  }
  pins.push_back(_white.getPin());
  return pins;
}

std::vector<float> Colorspace::getMaxValues(void) {
  std::vector<float> maxvals;
  for (int i=0; i<_LEDs.size(); i++) {
    maxvals.push_back(_LEDs[i].getMax());
  }
  maxvals.push_back(_white.getMax());
  return maxvals;
}

// And this is the meat. Converts the abstract color into RGBW (scaled 0-1).
std::vector<float> Colorspace::Hue2LEDs(HSIColor &HSI) {
  float H = fmod(HSI.getHue()+360,360);
  float S = HSI.getSaturation();
  float I = HSI.getIntensity();
  
  float tanH = tan(M_PI*fmod(H,360)/(float)180); // Get the tangent since we will use it often.
  
  // Has all LED output values followed by white.
  std::vector<float> LEDOutputs;
  
  for (int i=0; i<(_LEDs.size()+1); i++) {
    LEDOutputs.push_back(0);
  }
  
  int LED1, LED2;
 
  // Check the range to determine which intersection to do.
  // For angle less than the smallest CIE hue or larger than the largest, special case.
  
  if ((H < _angle[0]) || (H >= _angle[_LEDs.size()-1])) {
    // Then we're mixing the lowest angle LED with the highest angle LED.
    LED1 = _LEDs.size() - 1;
    LED2 = 0;
  }
  
  else {
    // Iterate through the angles until we find an LED with hue smaller than the angle.
    int i;
    for (i=1; (H > _angle[i]) && (i<(_LEDs.size()-1)); i++) {
      if (H > _angle[i]) {
        LED1 = i-1;
        LED2 = i;
      }
    }
    LED1 = i-1;
    LED2 = i;
  }
  
  // Get the ustar and vstar values for the target LEDs.
  float LED1_ustar = _LEDs[LED1].getU() - _white.getU();
  float LED1_vstar = _LEDs[LED1].getV() - _white.getV();
  float LED2_ustar = _LEDs[LED2].getU() - _white.getU();
  float LED2_vstar = _LEDs[LED2].getV() - _white.getV();
  
  // Get the slope between LED1 and LED2.
  float slope = _slope[LED1];
  
  float ustar = (LED2_vstar - slope*LED2_ustar)/(tanH - slope);
  float vstar = tanH/(slope - tanH) * (slope * LED2_ustar - LED2_vstar);
  
  // Set the two selected colors.
  LEDOutputs[LED1] = I * S * abs(ustar-LED2_ustar)/abs(LED2_ustar - LED1_ustar);
  LEDOutputs[LED2] = I * S * abs(ustar-LED1_ustar)/abs(LED2_ustar - LED1_ustar);
  
  // And set white.
  LEDOutputs[_LEDs.size()] = I * (1 - S);
  
//  // For debugging, print the actual output values.
//  Serial.println("Target Hue of " + String(H) + " between LEDs " + String(LED1) + " and " + String(LED2));
//  Serial.println("Output Values");
//  for (std::vector<float>::iterator i=LEDOutputs.begin(); i != LEDOutputs.end(); ++i) {
//    Serial.print(*i, 2);
//    Serial.print(" ");
//  }
//  Serial.println("");
  
  return LEDOutputs;
}


