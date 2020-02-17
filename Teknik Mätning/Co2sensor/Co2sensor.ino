#include <SoftwareSerial.h>
const int analogPin = A0; 
const int pwmPin = 10;
const long samplePeriod = 10000;

SoftwareSerial sensor(13, 15); // RX, TX

const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];
long lastSampleTime = 0;

void setup() {
  Serial.begin(9600);
  sensor.begin(9600);
  pinMode(pwmPin, INPUT_PULLUP);
  Serial.print("MHZ14");
}

void loop() {
  long now = millis();
  if (now > lastSampleTime + samplePeriod) {
    lastSampleTime = now;
    int ppmV; // = readPPMV();
    int ppmS = readPPMSerial();
    int ppmPWM; //= readPPMPWM();
    Serial.print(ppmV);
    Serial.print("\t");
    Serial.print(ppmPWM);
    Serial.print("\t");
    Serial.print(ppmS);
  }
}

int readPPMV() {
  float v = analogRead(analogPin) * 5.0 / 1023.0;
  int ppm = int((v - 0.4) * 3125.0);
  return ppm;
}

int readPPMSerial() {
  for (int i = 0; i < 9; i++) {
    sensor.write(requestReading[i]);
  }
  Serial.println("sent request");
  while (sensor.available() < 9) {yield();}
  for (int i =0; i < 9; i++) {
    result[i] = sensor.read();
  }
  int high = result[2];
  int low = result[3];
  //Serial.print(high); Serial.print(" "); Serial.println(low);
  return high * 256 + low;
}

int readPPMPWM() {
  while (digitalRead(pwmPin) == LOW) {yield();}; //wait for pulse to go high
  long t0 = millis();
  while (digitalRead(pwmPin) == HIGH) {yield();}; //wait for pulse to go low
  long t1 = millis();
  while (digitalRead(pwmPin) == LOW) {yield();}; //wait for pulse to go high again
  long t2 = millis();
  long th = t1 - t0;
  long tl = t2 - t1;
  long ppm = 5000L * (th - 2) / (th + t1 - 4);
  while (digitalRead(pwmPin) == HIGH) {yield();}; //wait for pulse to go low
  delay(10); // allow output to settle.
  return int(ppm);
}

