#define redpin 6
#define greenpin 22
#define bluepin 23
#define whitepin 9
#define resolution 16
#define PWMfrequency 183.106

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println("Starting Teensy");
    
  // put your setup code here, to run once:
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  pinMode(whitepin, OUTPUT);
  
  analogWriteFrequency(redpin, PWMfrequency);
  analogWriteFrequency(greenpin, PWMfrequency);
  analogWriteFrequency(bluepin, PWMfrequency);
  analogWriteFrequency(whitepin, PWMfrequency);
  analogWriteResolution(resolution);
  
  analogWrite(redpin, 0);
  analogWrite(greenpin, 0);
  analogWrite(bluepin, 0);
  analogWrite(whitepin, 0);
  
  while(1) {
    analogWrite(whitepin, 0);
    analogWrite(redpin, 0xFFFF);
    delay(500);
    analogWrite(redpin, 0);
    analogWrite(greenpin, 0xFFFF);
    delay(500);
    analogWrite(greenpin, 0);
    analogWrite(bluepin, 0xFFFF);
    delay(500);
    analogWrite(bluepin, 0);
    analogWrite(whitepin, 0xFFFF);
    delay(500);
  }  
}

void loop() {
  if (Serial.available()) evaluatecommand(Serial.readStringUntil(0x0D));
}

void evaluatecommand(String commandstring) {
  analogWrite(redpin, commandstring.toInt());
  analogWrite(greenpin, commandstring.toInt());
  analogWrite(bluepin, commandstring.toInt());
  analogWrite(whitepin, commandstring.toInt());
  Serial.println("Setting All Values to: " + String(commandstring.toInt(), HEX));
}
