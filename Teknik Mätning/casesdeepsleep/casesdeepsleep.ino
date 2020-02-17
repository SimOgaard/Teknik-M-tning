#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>   
#include <SoftwareSerial.h>
#include <Wire.h>
#include <AM2320.h>
#include <EEPROM.h>
AM2320 sensorT;
SoftwareSerial sensor(13, 15);
#define EEPROM_SIZE 1

int analogPin = A0;
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];
int Temp, Hum, Lux, Co2 = 0;
const String host = "car9o7qv7j.execute-api.us-east-1.amazonaws.com";
const String stage = "iot";
const char *fingerprint = "72:D4:00:92:77:37:50:C9:9B:A1:38:FA:21:8A:9B:FD:BA:CF:CD:49";
int updateInterval = 10000;
String macQuery;
enum states {
  findESP,
  foundESP
};
struct Response {
  int statusCode = -1;
  String payload = "";
};
states State;

WiFiClientSecure client;

Response makeRequest(String type, String uri, String query, String payload){
  if(client.connect(host, 443)){
    Serial.println(host+type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    Serial.println(payload);
    client.println(type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    client.println("Host: " + host);
    client.println("Connection: close");
    if(payload.length() > 0){
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.println(payload);
      client.println();
    }else{
      client.println();
    }
    String line;
    Response res;
    while(client.connected()){
      line = client.readStringUntil('\n');
      if(line.startsWith("HTTP") && res.statusCode < 0){
        res.statusCode = line.substring(9, 12).toInt();
      }if(line.startsWith("{")){
        client.stop();
        res.payload = line;
      }
    }
    return res;
  }else{
    Serial.println("Misslyckades med att ansluta till AWS.");
    Response res;
    res.statusCode = -1;
    res.payload = "no connection";
    return res;
  }
}

int readPPMSerial() {
  for (int i = 0; i < 9; i++) { sensor.write(requestReading[i]); }
  while (sensor.available() < 9) { }
  for (int i =0; i < 9; i++) { result[i] = sensor.read(); }
  return int(result[2]) * 256 + int(result[3]);
}

void getTemperature() {
  if (sensorT.measure()) {
    Temp = sensorT.getTemperature();
    Hum = sensorT.getHumidity();
  }
  else {
    int errorCode = sensorT.getErrorCode();
    switch (errorCode) {
      case 1: Serial.println("ERR: Sensor is offline"); break;
      case 2: Serial.println("ERR: CRC validation failed."); break;
    }
  }
}

void Connect(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("Simon");
  macQuery = "MAC=" + WiFi.macAddress();
  client.setFingerprint(fingerprint);
  Response getRes = makeRequest("GET", "/device", macQuery + "&Concise=true", "");
  if(getRes.statusCode == 404){
    Serial.println("Ingen info hittades, skapar ny enhet...");
    Response postRes = makeRequest("POST", "/device", macQuery, "");
    if(postRes.statusCode == 201){
      Serial.println("Enhet skapad, POSTMAAAN");
      //sleep ZZzz
    }
  }else if(getRes.statusCode == 200){
    Serial.print("Enhet hittades ");
    int registeredIndex = getRes.payload.indexOf("\"isRegistered\":");
    int updateIntervalIndex = getRes.payload.indexOf("\"updateInterval\":");
    if(registeredIndex >= 0 && updateIntervalIndex >= 0){
      int registeredStart = registeredIndex + 15;
      String registered = getRes.payload.substring(registeredStart, registeredStart + 5);
      int updateIntervalStart = updateIntervalIndex + 17;
      int updateIntervalEnd = getRes.payload.indexOf(',', updateIntervalStart);
      updateInterval = getRes.payload.substring(updateIntervalStart, updateIntervalEnd).toInt() * 1000;
      if(registered.startsWith("true")){
        Serial.print("och är registrerad, påbörjar mätning med intervallet "+String(updateInterval)+" millisekunder...");
        EEPROM.write(0, 1);
        EEPROM.commit();
      }
    }else{
      Serial.println("och är inte registrerad, POSTMAAAAN");
      //sleep ZZzz
    }
  }
}

void Send(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("Simon");
  macQuery = "MAC=" + WiFi.macAddress();
  client.setFingerprint(fingerprint);
  String payload = "{\"Temp\":" + String(Temp) + ",\"Hum\":" + String(Hum) + ",\"LDR\":" + String(Lux) + ",\"CO2\":" + String(Co2) + "}";
  makeRequest("PUT", "/device/data", macQuery, payload);
}

void getValues(){
  Lux = analogRead(analogPin);
  Co2 = readPPMSerial();
  getTemperature();
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(9600);
  sensor.begin(9600);
  Wire.begin(12,14);
}

void loop(){
//  delay(updateInterval);
  State=(states)EEPROM.read(0);
  switch (State){
    case findESP:
      Connect();
    break;
    case foundESP:
      getValues();
      Send();
    break;
  }
  //sleep ZZzz
}
