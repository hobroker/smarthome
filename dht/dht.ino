#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "Sweet Home 2.4GHz";
const char *password = "#magic13";

#define DHTPIN 5
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;

unsigned long previousMillis = 0;
const long interval = 10 * 1000;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();
    root["temperature"] = temperature;
    root["humidity"] = humidity;
    serializeJson(doc, *response);
    request->send(response);
  });

  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *request) {
    String response = "";
    response += F("home_temperature ") + String(temperature);
    response += F("\n");
    response += F("home_humidity ") + String(humidity);
    request->send(200, "text/html", response);
  });

  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    float newT = dht.readTemperature();
    if (isnan(newT)) {
      Serial.println("Failed to read temperature from DHT sensor!");
    } else {
      temperature = newT;
      Serial.print("Temperature: ");
      Serial.println(temperature);
    }

    float newH = dht.readHumidity();
    if (isnan(newH)) {
      Serial.println("Failed to read humidity from DHT sensor!");
    } else {
      humidity = newH;
      Serial.print("Humidity: ");
      Serial.println(humidity);
    }
  }
}