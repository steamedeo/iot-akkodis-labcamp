#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#include "./secrets.h"
#include "./utils.h"
#include "./base64.h"
#include "./sha256.h"

WiFiSSLClient wifiClient;

static const char IOT_EVENT_TOPIC[] = "devices/{device_id}/messages/events/";
String iotHubHost;
String deviceId;
String sharedAccessKey;

PubSubClient *mqtt_client = NULL;

#define SENSOR_READ_INTERVAL 2500
long lastSensorReadMillis = 0;

void setup() {
  Serial.begin(9600);

  connectToWiFi();
  splitConnectionString();

  // create SAS token and user name for connecting to MQTT broker
  String url = iotHubHost + urlEncode(String("/devices/" + deviceId).c_str());
  char *devKey = (char *)sharedAccessKey.c_str();
  long expire = getNow() + 864000;
  String sasToken = createIotHubSASToken(devKey, url, expire);
  String username = iotHubHost + "/" + deviceId + "/api-version=2016-11-14";

  // connect to the IoT Hub MQTT broker
  wifiClient.connect(iotHubHost.c_str(), 8883);
  mqtt_client = new PubSubClient(iotHubHost.c_str(), 8883, wifiClient);
  connectMQTT(deviceId, username, sasToken);

  // initialize timers
  lastSensorReadMillis = millis();
}

void loop() {
  if (mqtt_client->connected()) {
    mqtt_client->loop();

    // read sensor value
    if (millis() - lastSensorReadMillis > SENSOR_READ_INTERVAL) {
      float temperature = readTemperature();

      Serial.println("Sending telemetry...");
      
      String topic = (String)IOT_EVENT_TOPIC;
      topic.replace("{device_id}", deviceId);
      String payload = "{\"temperature\": {temperature}}";
      payload.replace("{temperature}", String(temperature));

      serial_printf("\t%s\n", payload.c_str());

      mqtt_client->publish(topic.c_str(), payload.c_str());

      lastSensorReadMillis = millis();
    }
  }
}

float readTemperature() {
  const int sensorPin = A0;

  int sensorValue = analogRead(sensorPin);
  float voltage = (sensorValue / 1024.0) * 5.0;
  float temperature = (voltage - 0.5) * 100;

  serial_printf("Read sensor value: [%d - %fV - %fC]", sensorValue, voltage, temperature);

  return temperature;
}

void connectToWiFi() {
  serial_printf("WiFi Firmware version is %s\n", WiFi.firmwareVersion());
  
  int status = WL_IDLE_STATUS;
  
  serial_printf("Attempting to connect to WiFi SSID: %s\n", WIFI_SSID);
  status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (status != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
}

void splitConnectionString() {
  String connectionString = (String)IOT_HUB_CONNECTION_STRING;

  int hostIndex = connectionString.indexOf("HostName=");
  int deviceIdIndex = connectionString.indexOf(";DeviceId=");
  int sharedAccessKeyIndex = connectionString.indexOf(";SharedAccessKey=");

  iotHubHost = connectionString.substring(hostIndex + 9, deviceIdIndex);
  deviceId = connectionString.substring(deviceIdIndex + 10, sharedAccessKeyIndex);
  sharedAccessKey = connectionString.substring(sharedAccessKeyIndex + 17);
}

String createIotHubSASToken(char *key, String url, long expire) {
  url.toLowerCase();

  String stringToSign = url + "\n" + String(expire);
  int keyLength = strlen(key);

  int decodedKeyLength = base64_dec_len(key, keyLength);
  char decodedKey[decodedKeyLength];

  base64_decode(decodedKey, key, keyLength);

  Sha256 *sha256 = new Sha256();
  sha256->initHmac((const uint8_t*)decodedKey, (size_t)decodedKeyLength);
  sha256->print(stringToSign);

  char* sign = (char*)sha256->resultHmac();
  int encodedSignLength = base64_enc_len(HASH_LENGTH);
  char encodedSign[encodedSignLength];
  base64_encode(encodedSign, sign, HASH_LENGTH);
  delete(sha256);

  return "SharedAccessSignature sr=" + url + "&sig=" + urlEncode((const char*)encodedSign) + "&se=" + String(expire);
}

void connectMQTT(String deviceId, String username, String password) {
  mqtt_client->disconnect();

  Serial.println("Starting IoT Hub connection");
  int retry = 0;
  while (retry < 10 && !mqtt_client->connected()) {
    if (mqtt_client->connect(deviceId.c_str(), username.c_str(), password.c_str())) {
      Serial.println("===> mqtt connected");
    } else {
      Serial.print("---> mqtt failed, rc=");
      Serial.println(mqtt_client->state());
      delay(2000);
      retry++;
    }
  }
}

unsigned long getNow() {
  const int NTP_PACKET_SIZE = 48;
  byte packetBuffer[NTP_PACKET_SIZE];

  IPAddress address(129, 6, 15, 28);

  WiFiUDP Udp;
  Udp.begin(2390);

  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;     // LI, Version, Mode
  packetBuffer[1] = 0;              // Stratum, or type of clock
  packetBuffer[2] = 6;              // Polling Interval
  packetBuffer[3] = 0xEC;           // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();

  // wait to see if a reply is available
  int waitCount = 0;
  while (waitCount < 20) {
    delay(500);
    waitCount++;
    if (Udp.parsePacket() ) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;

      Udp.stop();
      return (secsSince1900 - 2208988800UL);
    }
  }
  return 0;
}