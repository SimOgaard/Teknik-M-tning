#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>   
#include <SoftwareSerial.h>
#include <Wire.h>
#include <AM2320.h>
AM2320 sensorT;
SoftwareSerial sensor(13, 15);

int analogPin = A0;
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];
int Temp, Hum, Lux, Co2 = 0;
String host = "car9o7qv7j.execute-api.us-east-1.amazonaws.com";
const String stage = "iot";
const char *fingerprint = "72:D4:00:92:77:37:50:C9:9B:A1:38:FA:21:8A:9B:FD:BA:CF:CD:49";
bool isRegistered = false;
int updateInterval = 9000;
String macQuery;
struct Response {
  int statusCode = -1;
  String payload = "";
};

WiFiClientSecure client;

Response makeRequest(String type, String uri, String query, String payload){
  if(client.connect(host, 443)){
    Serial.println(host + type + " /" + stage + uri + "?" + query + " HTTP/1.1");
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
  } else{
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

void connectStart(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("Simon");
  macQuery = "MAC=" + WiFi.macAddress();
  client.setFingerprint(fingerprint);
}

void infoSend(){
  String payload = "{\"Temp\":" + String(Temp) + ",\"Hum\":" + String(Hum) + ",\"LDR\":" + String(Lux) + ",\"CO2\":" + String(Co2) + "}";
  makeRequest("PUT", "/device/data", macQuery, payload);
}

void infoCheck(){
  Response getRes = makeRequest("GET", "/device", macQuery + "&Concise=true", "");
  if(getRes.statusCode == 404){
    Response postRes = makeRequest("POST", "/device", macQuery, "");
  }else if(getRes.statusCode == 200){
    updateInterval = getRes.payload.substring(getRes.payload.indexOf("\"updateInterval\":") + 17, getRes.payload.indexOf(',', getRes.payload.indexOf("\"updateInterval\":") + 17)).toInt();
    Serial.print(updateInterval);
    infoSend();
  }
}

void timeGet(){
  host = "http://worldtimeapi.org/api/timezone/Europe";
  Response getRes = makeRequest("GET", "/Stockholm", "", "");
  Serial.print(client.readStringUntil('\n'));
  host = "car9o7qv7j.execute-api.us-east-1.amazonaws.com";
}

void values(){
  Lux = analogRead(analogPin);
  Co2 = readPPMSerial();
  getTemperature();
}

void setup() {
  Serial.begin(9600);
  sensor.begin(9600);
  Wire.begin(12,14);
}

void loop(){
//  delay(180000);
  timeGet();
  values();
  connectStart();
  infoCheck();
  ESP.deepSleep(updateInterval*1000);
}
