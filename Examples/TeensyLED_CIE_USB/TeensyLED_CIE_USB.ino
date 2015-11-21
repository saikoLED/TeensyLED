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

#define propgain 0.001

// Define the physical LEDs and their CIE LUV color locations.
// u', v', maxvalue, physical pin
CIELED white(0.202531646, 0.469936709, (float)30/180, 9);
CIELED red(0.5137017676, 0.5229440531, (float)30/78, 6);
CIELED amber(0.3135687079, 0.5529418124, (float)30/60, 5);
CIELED green(0.0595846867, 0.574988823, (float)30/125, 22);
CIELED cyan(0.0306675939, 0.5170937486, (float)30/95, 3);
CIELED blue(0.1747943747, 0.1117834986, (float)30/30, 23);
CIELED violet(0.31, 0.1, (float)30/30, 4);

// Create the physical abstraction for the LED controller.
// Redpin, Greenpin, Bluepin, Whitepin, Resolution, Frequency.
RGBWLamp lamp(6, (float)30/78, 22, (float)30/125, 23, 1, 9, (float)30/180, 16, 183.106);

// Creates a blank HSI color.
HSIColor color;

// Creates a freerunning HSI Fader.
HSIFader fader(HSIColor(0, 1, 0), HSIColor(120, 1, 0), 1000, 0);

// Creates a freerunning strober.
HSIStrober strober(HSIColor(), HSIColor(), 1000);

void setup() {
  Serial.begin(115200);
  
  pinMode(white.getPin(), OUTPUT);
  pinMode(red.getPin(), OUTPUT);
  pinMode(amber.getPin(), OUTPUT);
  pinMode(green.getPin(), OUTPUT);
  pinMode(cyan.getPin(), OUTPUT);
  pinMode(blue.getPin(), OUTPUT);
  pinMode(violet.getPin(), OUTPUT);
    
  while(1) {
    analogWrite(violet.getPin(), 0);
    analogWrite(white.getPin(), 0xFFFF);
    delay(1000);
    analogWrite(white.getPin(), 0);
    analogWrite(red.getPin(), 0xFFFF);
    delay(1000);
    analogWrite(red.getPin(), 0);
    analogWrite(amber.getPin(), 0xFFFF);
    delay(1000);
    analogWrite(amber.getPin(), 0);
    analogWrite(green.getPin(), 0xFFFF);
    delay(1000);
    analogWrite(green.getPin(), 0);
    analogWrite(cyan.getPin(), 0xFFFF);
    delay(1000);
    analogWrite(cyan.getPin(), 0);
    analogWrite(blue.getPin(), 0xFFFF);
    delay(1000);
    analogWrite(blue.getPin(), 0);
    analogWrite(violet.getPin(), 0xFFFF);
    delay(1000);
  }
  
  // Wait a bit on startup to let the USB interface come up.
  delay(1000);
  
  lamp.begin();
  
  // Initialize to fully saturated red with no intensity.
  color.setHSI(0, 1, 0);
  lamp.setColor(color);
  
//  fader.setFader(HSIColor(0, 1, 0), HSIColor(120, 1, 1), 5000, 0);
//  while(fader.isRunning()) {
//    handleFade();
//  }
}

boolean updated = false;
enum mode {HSI = 0, Strobe = 1, Fade = 2} mode = HSI;
float strobe;
enum strobestate {on = 1, off = 0} strobestate = on;
float savedintensity;

void loop() {
  if (Serial.available()) evaluateCommand(Serial.readStringUntil(0x0D));
  
  switch (mode) {
    case 0: // Standard HSI mode.
      handleHSI();
      break;
    case 1: // Handle Strober mode.
      handleStrobe();
      break;
    case 2: // Fade HSI mode.
      handleFade();
      break;
  }
}

void handleHSI() {
  if (color.isupdated()) {
    lamp.setColor(color);
  }
}

void handleFade() {
  // If the fader is running, update it.
  if (fader.isRunning()) {
    HSIColor newcolor = fader.getHSIColor();
    lamp.setColor(newcolor);
  }
  // If the fader is done, set state back to HSI.
  else mode = HSI;
}

void handleStrobe() {
  HSIColor newcolor = strober.getHSIColor();
  lamp.setColor(newcolor);
}

void evaluateCommand(String commandstring) {
  if (commandstring.startsWith("HSI ")) {
    // If it matches HSI, delete the command and capture three floats.
    commandstring.replace("HSI ", "");
    // Next, find the 2 spaces between the floats.
    int spaceIndex = commandstring.indexOf(' ');
    if (spaceIndex != -1) {
      int spaceIndex2 = commandstring.indexOf(' ', spaceIndex + 1);
      if (spaceIndex2 != -1) {
        // Then check that the three values are floats.
        if (checkFloat(commandstring.substring(0, spaceIndex)) == 0) {
          if (checkFloat(commandstring.substring(spaceIndex+1, spaceIndex2)) == 0) {
            if (checkFloat(commandstring.substring(spaceIndex2+1)) == 0) {
              color.setHSI(commandstring.substring(0, spaceIndex).toFloat(), commandstring.substring(spaceIndex+1, spaceIndex2).toFloat(), commandstring.substring(spaceIndex2+1).toFloat());
              mode = HSI;
              updated = true;
              Serial.println("OK");
            }
            else Serial.println("ERROR");
          }
          else Serial.println("ERROR");
        }
        else Serial.println("ERROR");
      }
      else Serial.println("ERROR");
    }
    else Serial.println("ERROR");
  }
  // Strobe command. HSI value 1, HSI value 2, period.
  else if (commandstring.startsWith("Strobe ")) {
    commandstring.replace("Strobe ", "");
    // First find the six expected spaces.
    int spaceIndex = commandstring.indexOf(' ');
    if (spaceIndex != -1) {
      int spaceIndex2 = commandstring.indexOf(' ', spaceIndex + 1);
      if (spaceIndex2 != -1) {
        int spaceIndex3 = commandstring.indexOf(' ', spaceIndex2 + 1);
        if (spaceIndex3 != -1) {
          int spaceIndex4 = commandstring.indexOf(' ', spaceIndex3 + 1);
          if (spaceIndex4 != -1) {
            int spaceIndex5 = commandstring.indexOf(' ', spaceIndex4 + 1);
            if (spaceIndex5 != -1) {
              int spaceIndex6 = commandstring.indexOf(' ', spaceIndex5 + 1);
              if (spaceIndex6 != -1) {
                
                // Then check that the three values are floats.
                if (checkFloat(commandstring.substring(0, spaceIndex)) == 0) {
                  if (checkFloat(commandstring.substring(spaceIndex+1, spaceIndex2)) == 0) {
                    if (checkFloat(commandstring.substring(spaceIndex2+1, spaceIndex3)) == 0) {
                      if (checkFloat(commandstring.substring(spaceIndex3+1, spaceIndex4)) == 0) {
                        if (checkFloat(commandstring.substring(spaceIndex4+1, spaceIndex5)) == 0) {
                          if (checkFloat(commandstring.substring(spaceIndex5+1, spaceIndex6)) == 0) {
                            if (checkFloat(commandstring.substring(spaceIndex6+1)) == 0) {
                              unsigned long time = commandstring.substring(spaceIndex6+1).toFloat();
                              if (time > 0) {
                                // Then all the values are valid.
                                HSIColor color1(commandstring.substring(0, spaceIndex).toFloat(), commandstring.substring(spaceIndex+1, spaceIndex2).toFloat(), commandstring.substring(spaceIndex2+1, spaceIndex3).toFloat());
                                HSIColor color2(commandstring.substring(spaceIndex3+1, spaceIndex4).toFloat(), commandstring.substring(spaceIndex4+1, spaceIndex5).toFloat(), commandstring.substring(spaceIndex5+1, spaceIndex6).toFloat());
                                
                                strober.setStrober(color1, color2, time);
                                mode = Strobe;
                                Serial.println("OK");
                              }
                              else Serial.println("ERROR");
                            }
                            else Serial.println("ERROR");
                          }
                          else Serial.println("ERROR");
                        }
                        else Serial.println("ERROR");
                      }
                      else Serial.println("ERROR");
                    }
                    else Serial.println("ERROR");
                  }
                  else Serial.println("ERROR");
                }
                else Serial.println("ERROR");
              }
              else Serial.println("ERROR");
            }
            else Serial.println("ERROR");
          }
          else Serial.println("ERROR");
        }
        else Serial.println("ERROR");
      }
      else Serial.println("ERROR");
    }
    else Serial.println("ERROR");
  }
  
  else if (commandstring.startsWith("Fade ")) {
    commandstring.replace("Fade ", "");
    
    // First find the seven expected spaces.
    int spaceIndex = commandstring.indexOf(' ');
    if (spaceIndex != -1) {
      int spaceIndex2 = commandstring.indexOf(' ', spaceIndex + 1);
      if (spaceIndex2 != -1) {
        int spaceIndex3 = commandstring.indexOf(' ', spaceIndex2 + 1);
        if (spaceIndex3 != -1) {
          int spaceIndex4 = commandstring.indexOf(' ', spaceIndex3 + 1);
          if (spaceIndex4 != -1) {
            int spaceIndex5 = commandstring.indexOf(' ', spaceIndex4 + 1);
            if (spaceIndex5 != -1) {
              int spaceIndex6 = commandstring.indexOf(' ', spaceIndex5 + 1);
              if (spaceIndex6 != -1) {
                int spaceIndex7 = commandstring.indexOf(' ', spaceIndex6 + 1);
                if (spaceIndex7 != -1) {
                
                  // Then check that the three values are floats.
                  if (checkFloat(commandstring.substring(0, spaceIndex)) == 0) {
                    if (checkFloat(commandstring.substring(spaceIndex+1, spaceIndex2)) == 0) {
                      if (checkFloat(commandstring.substring(spaceIndex2+1, spaceIndex3)) == 0) {
                        if (checkFloat(commandstring.substring(spaceIndex3+1, spaceIndex4)) == 0) {
                          if (checkFloat(commandstring.substring(spaceIndex4+1, spaceIndex5)) == 0) {
                            if (checkFloat(commandstring.substring(spaceIndex5+1, spaceIndex6)) == 0) {
                              if (checkFloat(commandstring.substring(spaceIndex6+1, spaceIndex7)) == 0) {
                                if (checkInt(commandstring.substring(spaceIndex7+1)) == 0) {
                                  unsigned long time = commandstring.substring(spaceIndex6+1, spaceIndex7).toFloat();
                                  if (time > 0) {
                                    // Then all the values are valid.
                                    HSIColor color1(commandstring.substring(0, spaceIndex).toFloat(), commandstring.substring(spaceIndex+1, spaceIndex2).toFloat(), commandstring.substring(spaceIndex2+1, spaceIndex3).toFloat());
                                    HSIColor color2(commandstring.substring(spaceIndex3+1, spaceIndex4).toFloat(), commandstring.substring(spaceIndex4+1, spaceIndex5).toFloat(), commandstring.substring(spaceIndex5+1, spaceIndex6).toFloat());
                                    byte direction = commandstring.substring(spaceIndex7+1).toInt();
                                    fader.setFader(color1, color2, time, direction);
                                    mode = Fade;
                                    Serial.println("OK");
                                  }
                                  else Serial.println("ERROR");
                                }
                                else Serial.println("ERROR");
                              }
                              else Serial.println("ERROR");
                            }
                            else Serial.println("ERROR");
                          }
                          else Serial.println("ERROR");
                        }
                        else Serial.println("ERROR");
                      }
                      else Serial.println("ERROR");
                    }
                    else Serial.println("ERROR");
                  }
                  else Serial.println("ERROR");
                }
                else Serial.println("ERROR");
              }
              else Serial.println("ERROR");
            }
            else Serial.println("ERROR");
          }
          else Serial.println("ERROR");
        }
        else Serial.println("ERROR");
      }
      else Serial.println("ERROR");
    }
    else Serial.println("ERROR");
  }
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

int checkInt(String data) {
  // The simplest way to go about testing the string for only numerical inputs
  // since Arduino doesn't implement regex, is probably to count the number of 
  // characters and then count the number of instances of each valid number
  // character. i.e. [0-9] and the decimal point for a float after trimming
  // whitespace or newlines. Oh, plus the negative sign.
  
  data.trim();
  
  int runningtotal = 0;
  
  for (int j=0; j<data.length(); j++) {
    char activechar = data.charAt(j);
    // Test against valid cahracter list.
    if ((activechar == '-') ||
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
