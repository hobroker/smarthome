#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFiClient.h>
#include <ir_Kelvinator.h>
#include <PubSubClient.h>
#include "secrets.h"

// WiFi
const char *kSsid = SECRETS_WIFI_SSID;
const char *kPassword = SECRETS_WIFI_PASSWORD;

// MQTT Broker
const char *mqtt_broker = SECRETS_MQTT_BROKER;
const char *mqtt_username = SECRETS_MQTT_USERNAME;
const char *mqtt_password = SECRETS_MQTT_PASSWORD;
const int mqtt_port = SECRETS_MQTT_PORT;

// topics
const char *temperature_command_topic = "esp8266/ac/temperature/set";
const char *temperature_state_topic = "esp8266/ac/temperature";
const char *mode_command_topic = "esp8266/ac/mode/set";
const char *mode_state_topic = "esp8266/ac/mode";

const uint16_t kIrLed = 4;  // (D2) ESP8266 GPIO pin to use.
IRKelvinatorAC ac(kIrLed);

ESP8266WebServer server(80);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void handleRoot() {
  server.send(200, "text/html",
              "<html>"
              "</head>"
              "<body>"
              "<a href=\"on\"><button>ON</button></a>"
              "<a href=\"off\"><button>OFF</button></a>"
              "</body>"
              "</html>");
}

void handleOn() {
  ac.on();
  finishHandle();
}

void handleOff() {
  ac.off();
  finishHandle();
}

void finishHandle() {
  ac.send();
  handleRoot();
}

void printState() {
  // Display the settings.
  Serial.println("Kelvinator A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char *ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kKelvinatorStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void prepareAC() {
  // Serial.println("Default state of the remote.");
  // printState();
  Serial.println("Setting desired state for A/C.");
  ac.off();
  ac.setFan(1);
  ac.setMode(kKelvinatorCool);
  ac.setTemp(29);
  ac.setSwingVertical(false, kKelvinatorSwingVOff);
  ac.setSwingHorizontal(false);
  ac.setXFan(false);
  ac.setIonFilter(false);
  ac.setLight(true);
}

void prepareWiFi() {
  Serial.println("Connecting to WiFi");
  WiFi.begin(kSsid, kPassword);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());
}

void prepareMQTT() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void prepareHTTP() {
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();
  Serial.println("HTTP server started");
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  char message[50];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.print("Message: ");
  Serial.println(message);

  handleMessage(topic, message);

  Serial.println("-----------------------");
}

void handleMessage(char *topic, char *message) {
  bool isHandled = false;
  if (strcmp(topic, temperature_command_topic) == 0) {
    isHandled = handleSetTemperature(message);
  } else if (strcmp(topic, mode_command_topic) == 0) {
    isHandled = handleSetMode(message);
  }

  if (isHandled) {
    Serial.println("Sending IR");
    ac.send();
  }
}

bool handleSetTemperature(char *value) {
  ac.setTemp((uint8_t)atoi(value));
  return client.publish(temperature_state_topic, value);
}

bool handleSetMode(char *value) {
  if (strcmp(value, "off") == 0) {
    ac.off();
  }

  if (strcmp(value, "cool") == 0) {
    ac.setMode(kKelvinatorCool);
    ac.on();
  }

  return client.publish(mode_state_topic, value);
}

void handlePublishCurrentTemperature() {
  String tmp = String(ac.getTemp());
  char arr[3];
  tmp.toCharArray(arr, 3);

  client.publish(temperature_state_topic, arr);
}

void handlePublishCurrentMode() {
  client.publish(mode_state_topic, ac.getPower() ? "cool" : "off");
}

void publishFullACState() {
  handlePublishCurrentTemperature();
  handlePublishCurrentMode();
}

void subscribe() {
  client.subscribe(temperature_command_topic);
  client.subscribe(mode_command_topic);
}

void setup() {
  ac.begin();
  Serial.begin(115200);

  prepareWiFi();
  prepareMQTT();
  prepareAC();
  prepareHTTP();

  publishFullACState();
  subscribe();
}

void loop() {
  client.loop();
  server.handleClient();
}