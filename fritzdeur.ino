/*
 * ESP32 reads contact GPIO12 for high/low state
 * high = fridgedoor closed
 * low = fridgedoor open
 * State is posted to MQTT revspace/fritzdeur with message 'open' or 'closed'
 * 
 * Adapted from low quality example code by Sebastius
 * 2019-08-30
 */

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "OTA_PASSWORD.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

// Update these with values suitable for your network.

const char* ssid = "revspace-pub-2.4ghz";
const char* password = "";
const char* mqtt_server = "mosquitto.space.revspace.nl";

const int Schakelcontact = 12;

WiFiClient espClient;
PubSubClient client(espClient);

int state = -1;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Fritzdeur-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("revdebug/fritzdeur", "Hoi van de koelkastdeur");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int flash = 2;

void setup() {
  pinMode(Schakelcontact, INPUT);
  Serial.begin(115200);

  esp_task_wdt_init(30 /* seconds */, true);
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err == ESP_OK) {
    Serial.println("Watchdog \\o/");
  } else {
    Serial.println("Watchdog FAIL :(");
    flash = 3;
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  // setup OTA
  ArduinoOTA.setHostname("esp-fritzdeur");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (flash) {
    flash--;
    client.publish("revspace/fritzdeur", "open");
    delay(500);
    client.publish("revspace/fritzdeur", "closed");
    delay(500);
  }

  bool newstate=digitalRead(Schakelcontact);

  if (newstate!=state) {
    Serial.print("Wisselen");
    state=newstate;
    client.publish("revspace/fritzdeur", state ? "closed" : "open");
    delay(100);
  }
  ArduinoOTA.handle();

  esp_task_wdt_reset();
}
