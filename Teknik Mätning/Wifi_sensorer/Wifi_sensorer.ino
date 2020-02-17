#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h> 
#include <WiFiManager.h>

#include <SoftwareSerial.h>
#include <Wire.h>
#include <AM2320.h>
AM2320 sensorT;
SoftwareSerial sensor(13, 15);

const String host = "agafh7et1a.execute-api.us-east-1.amazonaws.com";
const String stage = "iot";
const char *fingerprint = "72:D4:00:92:77:37:50:C9:9B:A1:38:FA:21:8A:9B:FD:BA:CF:CD:49";

int analogPin = A0;
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];
int Temp, Hum, Lux, Co2 = 0;

struct Response
{
  int statusCode = -1;
  String payload = "";
};

WiFiClientSecure client;

bool isRegistered = false;
int updateInterval = 1800;

String macQuery;

//123
void printRegisterGuide()
{
  Serial.println(" 1. Besök ABB IoT-webbsidan på http://abbiot.eu-north-1.elasticbeanstalk.com/");
  Serial.println(" 2. Logga in med din skol-email");
  Serial.print(" 3. Hitta denna enhet i den oregistrerade listan, MAC-addressen är ");
  Serial.print(WiFi.macAddress());
  Serial.println();
  Serial.println(" 4. Registrera den, och starta sedan om denna enhet");
}
//123

void getTemperature() {
  if (sensorT.measure()) {
    Temp = sensorT.getTemperature();
    Hum = sensorT.getHumidity();
    Serial.println("Temperature: "+String(Temp));
    Serial.println("Humidity: "+String(Hum));
  } else {
    int errorCode = sensorT.getErrorCode();
    switch (errorCode) {
      case 1: Serial.println("ERR: Sensor is offline"); break;
      case 2: Serial.println("ERR: CRC validation failed."); break;
    }
  }
}

Response makeRequest(String type, String uri, String query, String payload) {
  if(client.connect(host, 443)) {
    Serial.println(host+type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    Serial.println(payload);
    client.println(type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    client.println("Host: " + host);
    client.println("Connection: close");
    if(payload.length() > 0) {
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.println(payload);
      client.println();
    } else {
      client.println();
    }
    String line;
    Response res;
    while(client.connected()) {
      line = client.readStringUntil('\n');
      if(line.startsWith("HTTP") && res.statusCode < 0) {
        res.statusCode = line.substring(9, 12).toInt();
      } if(line.startsWith("{")){
        client.stop();
        res.payload = line;
      }
    }
    return res;
  } else {
    Serial.println("Misslyckades med att ansluta till AWS.");
    Response res;
    res.statusCode = -1;
    res.payload = "no connection";
    return res;
  }
}


void setup() {
  Serial.begin(9600);
  Wire.begin(12,14);
  delay(1000);
  Serial.print("Ansluter till ");
  Serial.print(".");
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.print("Klar\nLokal IP-adress: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC-adress: ");
  Serial.println(WiFi.macAddress());
  macQuery = "MAC=" + WiFi.macAddress();
  client.setFingerprint(fingerprint);
  Serial.println("Hämtar info om denna enhet...");
  Response getRes = makeRequest("GET", "/device", macQuery + "&Concise=true", "");
  if(getRes.statusCode == 404){
    Serial.println("Ingen info hittades, skapar ny enhet...");
    Response postRes = makeRequest("POST", "/device", macQuery, "");
    if(postRes.statusCode == 201) {
      Serial.println("Enhet skapad, gör följande steg:");
      printRegisterGuide();
    }
  } else if(getRes.statusCode == 200){
    Serial.print("Enhet hittades ");
    int registeredIndex = getRes.payload.indexOf("\"isRegistered\":");
    int updateIntervalIndex = getRes.payload.indexOf("\"updateInterval\":");
    if(registeredIndex >= 0 && updateIntervalIndex >= 0) {
      int registeredStart = registeredIndex + 15;
      String registered = getRes.payload.substring(registeredStart, registeredStart + 5);
      int updateIntervalStart = updateIntervalIndex + 17;
      int updateIntervalEnd = getRes.payload.indexOf(',', updateIntervalStart);
      updateInterval = getRes.payload.substring(updateIntervalStart, updateIntervalEnd).toInt() * 1000;
      if(registered == "false") {
        printRegisterGuide();
      }
      else if(registered.startsWith("true")) {
        Serial.print("och är registrerad, påbörjar mätning med intervallet ");
        Serial.print(updateInterval);
        Serial.println(" millisekunder...");
        isRegistered = true;
      }
    } else{
      Serial.println("och är inte registrerad, gör följande steg:");
      printRegisterGuide();
    }
  }
}

void loop() {
  if(isRegistered) {
    delay(10000);
    Co2 = readPPMSerial();
    Lux = analogRead(analogPin);
    Serial.println("PPM: "+String(Co2));
    Serial.println("Lux: "+String(Lux));
    getTemperature();
    String payload = "{\"temperature\":" + String(Temp, 1) + ",\"humidity\":" + String(Hum, 1) + "}";
    Serial.print("Skickar data. Storlek: ");
    Serial.println(payload.length());
    makeRequest("PUT", "/device/data", macQuery, payload);
    Serial.print("Väntar ");
    Serial.print(updateInterval);
    Serial.println(" millisekunder.");
    delay(updateInterval);
  }
}

int readPPMSerial() {
  for (int i = 0; i < 9; i++) { sensor.write(requestReading[i]); }
  while (sensor.available() < 9) { yield(); }
  for (int i =0; i < 9; i++) { result[i] = sensor.read(); }
  return int(result[2]) * 256 + int(result[3]);
}
