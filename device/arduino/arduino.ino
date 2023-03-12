#include <ArduinoJson.h>

#define CALIBRATION_DURATION 5000
#define TELEMETRY_DELAY 5000
#define AREF_VOLTAGE 3.3

#define TEMPERATURE_PIN A1
#define LIGHT_PIN A0

int temperatureMinSensorValue, temperatureMaxSensorValue;
int lightMinSensorValue, lightMaxSensorValue;

void setup() {
  analogReference(EXTERNAL);
  
  Serial.begin(9600);

  performCalibration(TEMPERATURE_PIN, &temperatureMinSensorValue, &temperatureMaxSensorValue);
  performCalibration(LIGHT_PIN, &lightMinSensorValue, &lightMaxSensorValue);
  sendCalibrationMessage();
}

void loop() {
  delay(TELEMETRY_DELAY);

  int temperatureSensorValue = analogRead(TEMPERATURE_PIN);
  int lightSensorValue = analogRead(LIGHT_PIN);
  sendTelemetryMessage(temperatureSensorValue, lightSensorValue);
}

float convertSensorValueToTemperature(int sensorValue) {
  float voltage = sensorValue * AREF_VOLTAGE / 1024.0;
  float temperature = (voltage - 0.5) * 100.0;

  return temperature;
}

void performCalibration(uint8_t sensorPin, int* minSensorValue, int* maxSensorValue) {
  unsigned long endCalibrationMillis = millis() + CALIBRATION_DURATION;

  *minSensorValue = 1024;
  *maxSensorValue = 0;

  while (millis() < endCalibrationMillis) {
    int sensorValue = analogRead(sensorPin);

    if (sensorValue < *minSensorValue) {
      *minSensorValue = sensorValue;
    }

    if (sensorValue > *maxSensorValue) {
      *maxSensorValue = sensorValue;
    }
  }
}

void sendCalibrationMessage() {
  const int capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(2);
  StaticJsonDocument<capacity> doc;
  doc["messageType"] = "CALIBRATION";

  JsonObject temperature = doc.createNestedObject("temperature");
  temperature["minSensorValue"] = temperatureMinSensorValue;
  temperature["maxSensorValue"] = temperatureMaxSensorValue;
  temperature["maxValue"] = convertSensorValueToTemperature(temperatureMinSensorValue);
  temperature["minValue"] = convertSensorValueToTemperature(temperatureMaxSensorValue);

  JsonObject light = doc.createNestedObject("light");
  light["minSensorValue"] = lightMinSensorValue;
  light["maxSensorValue"] = lightMaxSensorValue;

  char output[256];
  serializeJson(doc, output);
  Serial.println(output);
}

void sendTelemetryMessage(int temperatureSensorValue, int lightSensorValue) {
  const int capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(2);
  StaticJsonDocument<capacity> doc;
  doc["messageType"] = "TELEMETRY";

  JsonObject temperature = doc.createNestedObject("temperature");
  temperature["sensorValue"] = temperatureSensorValue;
  temperature["value"] = convertSensorValueToTemperature(temperatureSensorValue);
  temperature["status"] = temperatureSensorValue < temperatureMinSensorValue ? "LOW" : (temperatureSensorValue > temperatureMaxSensorValue ? "HIGH" : "STABLE");

  JsonObject light = doc.createNestedObject("light");
  light["sensorValue"] = lightSensorValue;
  light["status"] = lightSensorValue < lightMinSensorValue ? "LOW" : (lightSensorValue > lightMaxSensorValue ? "HIGH" : "STABLE");

  char output[256];
  serializeJson(doc, output);
  Serial.println(output);
}