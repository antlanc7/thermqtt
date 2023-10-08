#include <Arduino.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

WiFiClient socket;
PubSubClient mqttClient("192.168.178.4", 1883, socket);
#define ONEWIRE_PIN 33
OneWire oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

time_t startTime;

bool convertToJson(const tm* src, JsonVariant dst) {
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", src);
  return dst.set(buf);
}

void setup() {
  Serial.begin(115200);
  pinMode(ONEWIRE_PIN, INPUT_PULLUP);
  sensors.begin();
  WiFi.begin("GetBy-1709", "52190805368069642852");
  WiFi.waitStatusBits(STA_HAS_IP_BIT, portMAX_DELAY);
  timeClient.begin();
  timeClient.forceUpdate();
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  startTime = timeClient.getEpochTime();
  do {
    mqttClient.connect("ciao");
  } while (!mqttClient.connected());
  Serial.println("Connected");
}

void loop() {
  timeClient.update();
  sensors.requestTemperatures();
  StaticJsonDocument<1024> doc;
  time_t currTime = timeClient.getEpochTime();
  doc["time"] = localtime(&currTime);
  doc["startTime"] = localtime(&startTime);
  doc["uptime"] = currTime - startTime;
  doc["temperature"] = sensors.getTempCByIndex(0);
  mqttClient.beginPublish("thermqtt", measureJson(doc), true);
  serializeJson(doc, mqttClient);
  serializeJsonPretty(doc, Serial);
  mqttClient.endPublish();
  mqttClient.loop();
  delay(1000);
}