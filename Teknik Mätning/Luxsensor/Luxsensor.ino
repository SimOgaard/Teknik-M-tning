int AI_Sensor = 0; 
int SensorValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(AI_Sensor, INPUT);
}

void loop() {
  SensorValue = analogRead(AI_Sensor);
  Serial.println(SensorValue);
  delay(500);
}
