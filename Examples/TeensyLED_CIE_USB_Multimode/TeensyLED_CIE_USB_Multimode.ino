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
#include <memory>

#define propgain 0.001

// Create the physical abstraction for the LED controller.
// Resolution, Frequency.
RGBWLamp lamp(16, 183.106);

// Creates a starting HSI color for carrying HSI mode options.
HSIColor color(0, 1, 0);

// Creates a freerunning HSI Fader.
HSIFader fader(HSIColor(0, 1, 0), HSIColor(120, 1, 0), 1000, 0);

// Creates a freerunning strober.
HSIStrober strober(HSIColor(), HSIColor(), 1000);

HSICycler cycler(HSIColor(0, 1, 0), 1000, 1);

RandomFader randomfader(1000);

void setup() {
  Serial.begin(115200);
  
  // Wait a bit on startup to let the USB interface come up.
  delay(1000);
  
  // Define the physical LEDs and their CIE LUV color locations.
  // u', v', maxvalue, physical pin
  CIELED white(0.202531646, 0.469936709, (float)180/180, 9);
  CIELED red(0.5137017676, 0.5229440531, (float)78/78, 6);
  CIELED amber(0.3135687079, 0.5529418124, (float)60/60, 5);
  CIELED green(0.0595846867, 0.574988823, (float)125/125, 22);
  CIELED cyan(0.0306675939, 0.5170937486, (float)95/95, 3);
  CIELED blue(0.1747943747, 0.1117834986, (float)30/30, 23);
  CIELED violet(0.35, 0.15, (float)30/30, 4);

  // For the LZC series RGB LED.  
//  CIELED white(0.202531646, 0.469936709, (float)480/480, 9);
//  CIELED red(0.5137017676, 0.5229440531, (float)210/210, 6);
//  CIELED green(0.0595846867, 0.574988823, (float)340/340, 22);
//  CIELED blue(0.1747943747, 0.1117834986, (float)80/80, 23);
  
  // Create a colorspace object that will be put into the abstract lamp.
  std::shared_ptr<Colorspace> colorspace (new Colorspace(white));
  
  // Add the CIE LED definitions to the colorspace.
  colorspace->addLED(red);
  colorspace->addLED(amber);
  colorspace->addLED(green);
  colorspace->addLED(cyan);
  colorspace->addLED(blue);

  // Create the lamp colorspace.
  lamp.addColorspace(colorspace);
  
  // Initialize the random fader.
  // Add the colored LEDs to randomly switch between.
  randomfader.addLED(red);
  randomfader.addLED(amber);
  randomfader.addLED(green);
  randomfader.addLED(cyan);
  randomfader.addLED(blue);
  
  // Add the Effect LED (blacklight) with probability of being on.
  randomfader.addEffectLED(violet, 0.2);
  
  // And initialize the lamp so that it is fully functional.
  lamp.begin();
  
  // And start up the cycler.
  cycler.setCycler(HSIColor(0, 1, 1), 1000, 1);
  randomfader.startRandom(4000);
}

enum mode {HSI = 0, Strobe = 1, Fade = 2, Cycle = 3, Random = 4} mode = Random;

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
    case 3: // Standalone Cycling mode.
      handleCycle();
      break;
    case 4: // Random LED shifting for art.
      handleRandom();
      break;
  }
}

void handleHSI() {
  lamp.setColor(color);
}

void handleFade() {
  // If the fader is running, update it.
  if (fader.isRunning()) {
    color = fader.getHSIColor();
    lamp.setColor(color);
  }
  // If the fader is done, set state back to HSI.
  else mode = HSI;
}

void handleStrobe() {
  color = strober.getHSIColor();
  lamp.setColor(color);
}

void handleCycle() {
  color = cycler.getHSIColor();
  lamp.setColor(color);
}

void handleRandom() {
  std::vector<float> LEDs = randomfader.getLEDs();
  std::vector<int> pins = randomfader.getPins();
  lamp.setLEDs(LEDs, pins);
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
  // Effect LED command.
  else if (commandstring.startsWith("Effect ")) {
    commandstring.replace("Effect ", "");
    if (checkInt(commandstring) == 0) {
      int effect = commandstring.toInt();
      if (effect == 0) {
        digitalWrite(4, LOW);
        mode = HSI;
      }
      else if (effect == 1) {
        digitalWrite(4, HIGH);
        mode = HSI;
      }
      else Serial.println("ERROR");
    }
    else Serial.println("ERROR");
  }
  // Random Fader command.
  else if (commandstring.startsWith("Random")) {
    mode = Random;
    HSIColor blank(0, 0, 0);
    lamp.setColor(blank);
    Serial.println("OK");
  }
  // Cycler command.
  else if (commandstring.startsWith("Cycler ")) {
    commandstring.replace("Cycler ", "");
    int spaceIndex = commandstring.indexOf(' ');
    if (spaceIndex != -1) {
      if (checkFloat(commandstring.substring(0, spaceIndex)) == 0) {
        if (checkInt(commandstring.substring(spaceIndex+1)) == 0) {
          mode = Cycle;
          cycler.setCycler(color, commandstring.substring(0, spaceIndex).toFloat(), commandstring.substring(spaceIndex+1).toInt());

          Serial.println("OK");
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
                                    int direction = commandstring.substring(spaceIndex7+1).toInt();
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
