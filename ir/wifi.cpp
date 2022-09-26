#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "secrets.h"

const char *ssid = SECRETS_WIFI_SSID;
const char *pass = SECRETS_WIFI_PASSWORD;

void prepareWiFi() {
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, pass);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());
}
