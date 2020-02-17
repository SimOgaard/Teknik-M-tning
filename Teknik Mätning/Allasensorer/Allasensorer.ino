#include <SoftwareSerial.h>
#include <Wire.h>
#include <AM2320.h>
AM2320 sensorT;
SoftwareSerial sensor(13, 15);

int analogPin = A0;
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];
int Temp, Hum, Lux, Co2 = 0;
int Ljusniva = 600;

void setup() {
  Serial.begin(9600);
  Wire.begin(12,14);
}

void getTemperature() {
  if (sensorT.measure()) {
    Temp = sensorT.getTemperature();
    Hum = sensorT.getHumidity();
    Serial.println("Temperature: "+String(Temp));
    Serial.println("Humidity: "+String(Hum));
  }
  else {
    int errorCode = sensorT.getErrorCode();
    switch (errorCode) {
      case 1: Serial.println("ERR: Sensor is offline"); break;
      case 2: Serial.println("ERR: CRC validation failed."); break;
    }
  }
}

void loop() {
  Lux = analogRead(analogPin);
  Co2 = readPPMSerial();
  Serial.println("PPM: "+String(Co2));
  Serial.println("Lux: "+String(Lux));
  getTemperature();
  ESP.deepSleep(10e6);
}

int readPPMSerial() {
  for (int i = 0; i < 9; i++) { sensor.write(requestReading[i]); }
  while (sensor.available() < 9) { yield(); }
  for (int i =0; i < 9; i++) { result[i] = sensor.read(); }
  return int(result[2]) * 256 + int(result[3]);
}
