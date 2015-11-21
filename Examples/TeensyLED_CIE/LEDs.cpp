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

// Hue Offset to account for physical red not being at 0 degrees in CIE LUV.
// This is used to make the user input easier.
#define RedBase 9.667447472

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

RGBWLamp::RGBWLamp(char redpin, char greenpin, char bluepin, char whitepin, char resolution, float PWMfrequency) :
  _redpin(redpin),
  _greenpin(greenpin),
  _bluepin(bluepin),
  _whitepin(whitepin),
  _resolution(resolution),
  _hue(0),
  _saturation(1),
  _intensity(0),
  _PWMfrequency(PWMfrequency) {
}

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
  float H = fmod(_hue+RedBase+360,360);
  float S = _saturation;
  float I = _intensity;
  
  float tanH = tan(M_PI*fmod(H,360)/(float)180); // Get the tangent since we will use it often.
  float ustar, vstar;
  
  boolean success = false;
  
  // Check the range to determine which intersection to do.
  
  if ((H >= RedAngle) && (H < GreenAngle)) {
    // Then we are finding the point between Red and Green with the right hue.
    ustar=(Green_vstar-RGm*Green_ustar)/(tanH - RGm);
    vstar=tanH/(RGm-tanH) *(RGm*Green_ustar - Green_vstar);
    r = S * abs(ustar-Green_ustar)/abs(Green_ustar - Red_ustar);
    g = S * abs(ustar-Red_ustar)/abs(Green_ustar - Red_ustar);
    b = 0;
    w = 1-S;
    success = true;
  }
  else if ((H >= GreenAngle) && (H < BlueAngle)) {
    // Then we are finding the point between Green and Blue with the right hue.
    ustar=(Blue_vstar-GBm*Blue_ustar)/(tanH - GBm);
    vstar=tanH/(GBm-tanH) *(GBm*Blue_ustar - Blue_vstar);
    r = 0;
    g = S * abs(ustar-Blue_ustar)/abs(Green_ustar - Blue_ustar);
    b = S * abs(ustar-Green_ustar)/abs(Green_ustar - Blue_ustar);
    w = 1-S;
    success = true;
  }
  else if (((H >= BlueAngle) && (H < 360)) || (H < RedAngle)) {
    // Then we are finding the point between Green and Blue with the right hue.
    ustar=(Red_vstar-BRm*Red_ustar)/(tanH - BRm);
    vstar=tanH/(BRm-tanH) *(BRm*Red_ustar - Red_vstar);
    r = S * abs(ustar-Blue_ustar)/abs(Red_ustar - Blue_ustar);
    g = 0;
    b = S * abs(ustar-Red_ustar)/abs(Red_ustar - Blue_ustar);
    w = 1-S;
    success = true;
  }
  else {
    // Something weird happened.
    ustar = 0;
    vstar = 0;
    r = 0;
    g = 0;
    b = 0;
    w = 0;
  }
  
  if (success) {
    Serial.println("Hue: " + String(H,3) + ", Tan: " + String(tanH,3) + ", (u', v'): (" + String(ustar+White_uprime,3) + ", " + String(vstar+White_vprime,3) + "), RGB: " + String((100*r),1) + "% " + String((100*g),1) + "% " + String((100*b),1) + "%");
    
    // Mapping Function from rgbw = [0:1] onto their respective ranges.
    // For standard use, this would be [0:1]->[0:0xFFFF] for instance.
    
    analogWrite(_redpin, ((1<<_resolution)-1)*(r*I*RedMax));
    analogWrite(_greenpin, ((1<<_resolution)-1)*(g*I*GreenMax));
    analogWrite(_bluepin, ((1<<_resolution)-1)*(b*I*BlueMax));
    analogWrite(_whitepin, ((1<<_resolution)-1)*(w*I*WhiteMax));
  }
}
