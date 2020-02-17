#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic    
#include <SoftwareSerial.h>
#include <AM2320.h>
#include <Wire.h>
AM2320 sensor1;
SoftwareSerial sensor(13, 15); // RX, TX

 

float hum =0;
float temp = 0;
byte analog=0;
byte result[9];
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

 

// Host URL for the API, do not change unless you move host
const String host = "car9o7qv7j.execute-api.us-east-1.amazonaws.com";

 

// API Stage to use, see the ABB-IoT API in AWS API Gateway
const String stage = "iot";

 

// Warning: This fingerprint expires December 14th 2019.
// To update it, view the SSL certificate in your browser:
//  1. Go to the host URL
//    "https://agafh7et1a.execute-api.us-east-1.amazonaws.com/"
//  2. Click the lock icon near the URL field
//  3. View the certificate
//  4. Copy the SHA-1 Fingerprint value to this string
const char *fingerprint = "72:D4:00:92:77:37:50:C9:9B:A1:38:FA:21:8A:9B:FD:BA:CF:CD:49";

 

// HTTP response object
struct Response
{
  // HTTP status code
  int statusCode = -1;
  // Response message
  String payload = "";
};

 

// HTTPS client
WiFiClientSecure client;

 

// Important values from database
bool isRegistered = false;
int updateInterval = 10000;

 

// Query string for HTTP requests
String macQuery;

 

// Temperature and humidity sensor object
// Adafruit_AM2320 am2320 = Adafruit_AM2320();

 

// Prints the steps to register a device
void printRegisterGuide()
{
  Serial.println(" 1. Besök ABB IoT-webbsidan på http://abbiot.eu-north-1.elasticbeanstalk.com/");
  Serial.println(" 2. Logga in med din skol-email");
  Serial.print(" 3. Hitta denna enhet i den oregistrerade listan, MAC-addressen är ");
  Serial.print(WiFi.macAddress());
  Serial.println();
  Serial.println(" 4. Registrera den, och starta sedan om denna enhet");
}

 

void MeasureData(){

 

if (isnan(sensor1.getHumidity())) {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
     hum = sensor1.getHumidity();
      Serial.print("Humidity: ");
      Serial.print(hum);
    Serial.println(F("%"));
  Serial.print("\t\t");
  }
  
  if (isnan(sensor1.getTemperature())) {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
      temp = sensor1.getTemperature();
      Serial.print("Temperature: ");
  Serial.print(temp);
    Serial.println(F("°C"));
  Serial.print("\t\t");
  }

 

  
}
int readPPMSerial() {
  for (int i = 0; i < 9; i++) {
    sensor.write(requestReading[i]); 
  }
  //Serial.println("sent request");
  while (sensor.available() < 9) {}; // wait for response
  for (int i = 0; i < 9; i++) {
    result[i] = sensor.read(); 
  }
  int high = result[2];
  int low = result[3];
    //Serial.print(high); Serial.print(" ");Serial.println(low);
  return high * 256 + low;
}

 

// Makes an HTTP request
// type: Request type (GET, PUT, POST, etc.)
// uri: Subdirectory
// query: Query string
// payload: Payload/Message in JSON format (Not for GET requests)
// Returns a Response object (see struct above)
Response makeRequest(String type, String uri, String query, String payload)
{
  // Connect to API host
  if(client.connect(host, 443))
  {
      Serial.println(host+type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    Serial.println(payload);
    // Writing HTTP request
    client.println(type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    client.println("Host: " + host);
    client.println("Connection: close");
    
    // If there's a payload, include content-length
    if(payload.length() > 0)
    {
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.println(payload);
      client.println();
    }
    else
    {
      client.println();
    }

 

    // An empty println means the end of the request

 

    // Placeholder for response data
    String line;
    // Response object
    Response res;
    // While connection is alive
    while(client.connected())
    {
      // Read one line
      line = client.readStringUntil('\n');

 

      // Print response to serial monitor
      // Serial.println(line);
      
      // A line starting with HTTP contains the status code
      if(line.startsWith("HTTP") && res.statusCode < 0)
      {
        // Take the appropriate substring and convert to an integer
        res.statusCode = line.substring(9, 12).toInt();
      }
      
      // If the line starts with a brace we have our response in JSON format
      if(line.startsWith("{"))
      {
        // End the connection
        client.stop();
        // Set the payload in our response object
        res.payload = line;
      }
    }

 

    // Return our response
    return res;
  }
  else
  {
    // Connection failed
    Serial.println("Misslyckades med att ansluta till AWS.");
    Response res;
    res.statusCode = -1;
    res.payload = "no connection";
    // Return a "no connection" response
    return res;
  }
}

 


void setup() {

//EEPROM.begin(EEPROM_SIZE);

  
  // Begin serial communication
  Wire.begin(12,14);
  Serial.begin(9600);
  sensor.begin(9600);
  pinMode(0, INPUT);
  delay(1000);

 

  Serial.print("Ansluter till ");
  Serial.print(".");
     WiFiManager wifiManager;
     wifiManager.autoConnect("AutoConnectAP");
  // Connecting to WiFi access point
  // WiFi.begin(ssid, password);

 

  // while(WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  
  Serial.print("Klar\nLokal IP-adress: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC-adress: ");
  Serial.println(WiFi.macAddress());

 

  // Set the MAC address query string
  macQuery = "MAC=" + WiFi.macAddress();

 

  // Sets the fingerprint for SSL encrypted connection
  client.setFingerprint(fingerprint);

 

  Serial.println("Hämtar info om denna enhet...");
  // Getting device info from API
  Response getRes = makeRequest("GET", "/device", macQuery + "&Concise=true", "");
  if(getRes.statusCode == 404) // Not Found
  {
    Serial.println("Ingen info hittades, skapar ny enhet...");

 

    // Adding device to database
    Response postRes = makeRequest("POST", "/device", macQuery, "");
    if(postRes.statusCode == 201) // Created
    {
      // Give instructions to user
      Serial.println("Enhet skapad, gör följande steg:");
      printRegisterGuide();
    }
  }
  else if(getRes.statusCode == 200) // OK
  {
    Serial.print("Enhet hittades ");

 

    // Find isRegistered and updateInterval
    int registeredIndex = getRes.payload.indexOf("\"isRegistered\":");
    int updateIntervalIndex = getRes.payload.indexOf("\"updateInterval\":");
    // If both keys are found
    if(registeredIndex >= 0 && updateIntervalIndex >= 0)
    {
      // Jump to isRegistered value
      int registeredStart = registeredIndex + 15;
      // Take the appropriate substring from the response
      // Since it's 5 characters, the response will be "false" or "true,"
      // The colon after "true" is the next key in the object
      String registered = getRes.payload.substring(registeredStart, registeredStart + 5);

 

      // Jump to updateInterval value
      int updateIntervalStart = updateIntervalIndex + 17;
      // Find the end of the value
      int updateIntervalEnd = getRes.payload.indexOf(',', updateIntervalStart);
      // Take the appropriate substring from the response
      updateInterval = getRes.payload.substring(updateIntervalStart, updateIntervalEnd).toInt() * 1000;
      
      // If not registered already
      if(registered == "false")
      {
        // Give instructions
        printRegisterGuide();
      }
      // If registered
      else if(registered.startsWith("true"))
      {
        Serial.print("och är registrerad, påbörjar mätning med intervallet ");
        Serial.print(updateInterval);
        Serial.println(" millisekunder...");
        // Begin measuring
        // am2320.begin();
        // Set isRegistered to true so the loop will run
        isRegistered = true;
      }
    }
    // If either key (isRegistered or updateInterval) is not found
    else
    {
      Serial.println("och är inte registrerad, gör följande steg:");
      // Give instructions
      printRegisterGuide();
    }
  }
}

 

void loop() {
  // Only measure and send data if registered
  //if(isRegistered)
  if (true != false){

 

    // Get current temperature and humidity
    //MeasureData();
    sensor1.measure();
    int humi = sensor1.getHumidity();
    int tempr = sensor1.getTemperature();
    int lux = analogRead(analog) ;
    int ppmS = readPPMSerial();
    Serial.print("Ppm CO2: ");
    Serial.println(ppmS);
    Serial.print("temp:");
    Serial.println(tempr);
    Serial.print("Hum:");
    Serial.println(humi);
    Serial.print("Lux:");
    Serial.println(lux);
    String payload = "{\"temperature\":" + String(tempr) + ",\"humidity\":" + String(humi) + ",\"LDR\":" + String(lux) + ",\"CO2\":" + String(ppmS) + "}";
    Serial.print("Skickar data. Storlek: ");
    Serial.println(payload.length());
    // Send the data
    makeRequest("PUT", "/device/data", macQuery, payload);
    Serial.print("Väntar ");
    Serial.print(updateInterval);
    Serial.println(" millisekunder.");
    delay(updateInterval);
  }
}
/*
//#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic    
// #include <Adafruit_AM2320.h>
//#include <Adafruit_Sensor.h>
//READ DHT 11 temp
//#include "DHT.h"

#include <SoftwareSerial.h>
#include <Wire.h>
#include <AM2320.h>
AM2320 sensorT;
SoftwareSerial sensor(13, 15);

//DHT dht;
//float humidity =0;
//float temperature = 0;
int analogPin = A0;
const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte result[9];
int Temp, Hum, Lux, Co2 = 0;


// Host URL for the API, do not change unless you move host
const String host = "agafh7et1a.execute-api.us-east-1.amazonaws.com";

// API Stage to use, see the ABB-IoT API in AWS API Gateway
const String stage = "iot";

// Warning: This fingerprint expires December 14th 2019.
// To update it, view the SSL certificate in your browser:
//  1. Go to the host URL
//    "https://agafh7et1a.execute-api.us-east-1.amazonaws.com/"
//  2. Click the lock icon near the URL field
//  3. View the certificate
//  4. Copy the SHA-1 Fingerprint value to this string
const char *fingerprint = "72:D4:00:92:77:37:50:C9:9B:A1:38:FA:21:8A:9B:FD:BA:CF:CD:49";

// HTTP response object
struct Response
{
  // HTTP status code
  int statusCode = -1;
  // Response message
  String payload = "";
};

// HTTPS client
WiFiClientSecure client;

// Important values from database
bool isRegistered = false;
int updateInterval = 1800;

// Query string for HTTP requests
String macQuery;

// Temperature and humidity sensor object
// Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Prints the steps to register a device
void printRegisterGuide()
{
  Serial.println(" 1. Besök ABB IoT-webbsidan på http://abbiot.eu-north-1.elasticbeanstalk.com/");
  Serial.println(" 2. Logga in med din skol-email");
  Serial.print(" 3. Hitta denna enhet i den oregistrerade listan, MAC-addressen är ");
  Serial.print(WiFi.macAddress());
  Serial.println();
  Serial.println(" 4. Registrera den, och starta sedan om denna enhet");
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

void MeasureData(){
  Lux = analogRead(analogPin);
//  if Lux > Ljusniva {
    Co2 = readPPMSerial();
    Serial.println("PPM: "+String(Co2));
    Serial.println("Lux: "+String(Lux));
    getTemperature();

    //skicka värderna
//  }

/*if (isnan(dht.getHumidity())) {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
     humidity = dht.getHumidity();
      Serial.print("Humidity: ");
      Serial.print(humidity, 1);
    Serial.println(F("%"));
  Serial.print("\t\t");
  }
  
  if (isnan(dht.getTemperature())) {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
      temperature = dht.getTemperature();
      Serial.print("Temperature: ");
  Serial.print(temperature, 1);
    Serial.println(F("°C"));
  Serial.print("\t\t");
  }

  
}


// Makes an HTTP request
// type: Request type (GET, PUT, POST, etc.)
// uri: Subdirectory
// query: Query string
// payload: Payload/Message in JSON format (Not for GET requests)
// Returns a Response object (see struct above)
Response makeRequest(String type, String uri, String query, String payload)
{
  // Connect to API host
  if(client.connect(host, 443))
  {
      Serial.println(host+type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    Serial.println(payload);
    // Writing HTTP request
    client.println(type + " /" + stage + uri + "?" + query + " HTTP/1.1");
    client.println("Host: " + host);
    client.println("Connection: close");
    
    // If there's a payload, include content-length
    if(payload.length() > 0)
    {
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.println(payload);
      client.println();
    }
    else
    {
      client.println();
    }

    // An empty println means the end of the request

    // Placeholder for response data
    String line;
    // Response object
    Response res;
    // While connection is alive
    while(client.connected())
    {
      // Read one line
      line = client.readStringUntil('\n');

      // Print response to serial monitor
      // Serial.println(line);
      
      // A line starting with HTTP contains the status code
      if(line.startsWith("HTTP") && res.statusCode < 0)
      {
        // Take the appropriate substring and convert to an integer
        res.statusCode = line.substring(9, 12).toInt();
      }
      
      // If the line starts with a brace we have our response in JSON format
      if(line.startsWith("{"))
      {
        // End the connection
        client.stop();
        // Set the payload in our response object
        res.payload = line;
      }
    }

    // Return our response
    return res;
  }
  else
  {
    // Connection failed
    Serial.println("Misslyckades med att ansluta till AWS.");
    Response res;
    res.statusCode = -1;
    res.payload = "no connection";
    // Return a "no connection" response
    return res;
  }
}


void setup() {
  // Begin serial communication
  Serial.begin(9600);
    //dht.setup(2); //Pin 2 DPin 4, <-Firstpin, Middlepin 3.3 v, Lastpin Grd.
    
  Wire.begin(12,14);
  // Clear monitor
  // Serial.flush();
  delay(1000);

  Serial.print("Ansluter till ");
  Serial.print(".");
     WiFiManager wifiManager;
     wifiManager.autoConnect("AutoConnectAP");
  // Connecting to WiFi access point
  // WiFi.begin(ssid, password);

  // while(WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  
  Serial.print("Klar\nLokal IP-adress: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC-adress: ");
  Serial.println(WiFi.macAddress());

  // Set the MAC address query string
  macQuery = "MAC=" + WiFi.macAddress();

  // Sets the fingerprint for SSL encrypted connection
  client.setFingerprint(fingerprint);

  Serial.println("Hämtar info om denna enhet...");
  // Getting device info from API
  Response getRes = makeRequest("GET", "/device", macQuery + "&Concise=true", "");
  if(getRes.statusCode == 404) // Not Found
  {
    Serial.println("Ingen info hittades, skapar ny enhet...");

    // Adding device to database
    Response postRes = makeRequest("POST", "/device", macQuery, "");
    if(postRes.statusCode == 201) // Created
    {
      // Give instructions to user
      Serial.println("Enhet skapad, gör följande steg:");
      printRegisterGuide();
    }
  }
  else if(getRes.statusCode == 200) // OK
  {
    Serial.print("Enhet hittades ");

    // Find isRegistered and updateInterval
    int registeredIndex = getRes.payload.indexOf("\"isRegistered\":");
    int updateIntervalIndex = getRes.payload.indexOf("\"updateInterval\":");
    // If both keys are found
    if(registeredIndex >= 0 && updateIntervalIndex >= 0)
    {
      // Jump to isRegistered value
      int registeredStart = registeredIndex + 15;
      // Take the appropriate substring from the response
      // Since it's 5 characters, the response will be "false" or "true,"
      // The colon after "true" is the next key in the object
      String registered = getRes.payload.substring(registeredStart, registeredStart + 5);

      // Jump to updateInterval value
      int updateIntervalStart = updateIntervalIndex + 17;
      // Find the end of the value
      int updateIntervalEnd = getRes.payload.indexOf(',', updateIntervalStart);
      // Take the appropriate substring from the response
      updateInterval = getRes.payload.substring(updateIntervalStart, updateIntervalEnd).toInt() * 1000;
      
      // If not registered already
      if(registered == "false")
      {
        // Give instructions
        printRegisterGuide();
      }
      // If registered
      else if(registered.startsWith("true"))
      {
        Serial.print("och är registrerad, påbörjar mätning med intervallet ");
        Serial.print(updateInterval);
        Serial.println(" millisekunder...");
        // Begin measuring
        // am2320.begin();
        // Set isRegistered to true so the loop will run
        isRegistered = true;
      }
    }
    // If either key (isRegistered or updateInterval) is not found
    else
    {
      Serial.println("och är inte registrerad, gör följande steg:");
      // Give instructions
      printRegisterGuide();
    }
  }
}

void loop() {
  // Only measure and send data if registered
  if(isRegistered)
  {

    // Get current temperature and humidity
    MeasureData();
    // If either value is NaN
    if(Temp != Temp || Hum != Hum || Lux != Lux || Co2 != Co2) {
      // The connection is loose, report to serial monitor
      Serial.println("Anslutningen till sensorn är glapp eller felgjord. Prövar igen om 20 sekunder.");
      delay(20000);
      return;
    }

    // Payload to send to API
    String payload = "{\"temperature\":" + String(Temp, 1) + ",\"humidity\":" + String(Hum, 1) + "}";
    Serial.print("Skickar data. Storlek: ");
    Serial.println(payload.length());
    // Send the data
    makeRequest("PUT", "/device/data", macQuery, payload);
    Serial.print("Väntar ");
    Serial.print(updateInterval);
    Serial.println(" millisekunder.");

    // Wait for next measurement
    delay(updateInterval);
//    ESP.deepSleep(10e6);
  }
}

int readPPMSerial() {
  for (int i = 0; i < 9; i++) { sensor.write(requestReading[i]); }
  while (sensor.available() < 9) { yield(); }
  for (int i =0; i < 9; i++) { result[i] = sensor.read(); }
  return int(result[2]) * 256 + int(result[3]);
}
*/
