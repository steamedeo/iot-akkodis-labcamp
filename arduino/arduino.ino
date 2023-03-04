const int temperatureSensorPin = A0;
const int lightSensorPin = A1;

int temperatureSensorHigh = 0;
int temperatureSensorLow = 1024;

int lightSensorHigh = 0;
int lightSensorLow = 1024;

void setup() {
  Serial.begin(9600);

  while (millis() < 5000) {
    int temperatureSensorValue = analogRead(temperatureSensorPin);
    if (temperatureSensorValue > temperatureSensorHigh) {
      temperatureSensorHigh = temperatureSensorValue;
    }
    if (temperatureSensorValue < temperatureSensorLow) {
      temperatureSensorLow = temperatureSensorValue;
    }

    int lightSensorValue = analogRead(lightSensorPin);
    if (lightSensorValue > lightSensorHigh) {
      lightSensorHigh = lightSensorValue;
    }
    if (lightSensorValue < lightSensorLow) {
      lightSensorLow = lightSensorValue;
    }
  }

  Serial.print("{ \"code\": \"INITIALIZATION\", \"temperature\": { \"min\": ");
  Serial.print(temperatureSensorLow);
  Serial.print(", \"max\": ");
  Serial.print(temperatureSensorHigh);
  Serial.print(" }, \"light\": { \"min\": ");
  Serial.print(lightSensorLow);
  Serial.print(", \"max\": ");
  Serial.print(lightSensorHigh);
  Serial.println(" } }");
}

void loop() {
  int temperatureSensorValue = analogRead(temperatureSensorPin);
  int lightSensorValue = analogRead(lightSensorPin);

  Serial.print("{ \"code\": \"TELEMETRY\", \"temperature\": { \"value\": ");
  Serial.print(temperatureSensorValue);
  Serial.print(", \"status\": ");
  if (temperatureSensorValue > temperatureSensorHigh) {
    Serial.print("\"HIGHER\" }, ");
  } else if (temperatureSensorValue < temperatureSensorLow) {
    Serial.print("\"LOWER\" }, ");
  } else {
    Serial.print("\"STABLE\" }, ");
  }
  Serial.print("\"light\": { \"value\": ");
  Serial.print(lightSensorValue);
  Serial.print(", \"status\": ");
  if (lightSensorValue > lightSensorHigh) {
    Serial.print("\"HIGHER\" } ");
  } else if (lightSensorValue < lightSensorLow) {
    Serial.print("\"LOWER\" } ");
  } else {
    Serial.print("\"STABLE\" } ");
  }
  Serial.println("}");
  
  delay(1000);
}
