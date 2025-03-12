#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "DaWiFi";
const char* password = "DaPassword123!";

// MQTT broker settings
const char* mqtt_broker = "192.168.8.27";
const int mqtt_port = 1883;
const char* mqtt_topic = "espresense/devices/pranav-phone/microwave";
const char* mqtt_username = "DaUser";
const char* mqtt_password = "DaPassword123!";


PubSubClient client(espClient);

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void connectToMQTT() {
  Serial.print("Connecting to MQTT...");
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println(" Connected!");
      if(client.subscribe(mqtt_topic)){
        Serial.println(" Subscribed!");
      }
    } else {
      Serial.print(" Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Payload: " + message);

  // Extract distance from JSON payload
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, message);
  if (!error) {
    float distance = doc["distance"];
    Serial.print("Extracted Distance: ");
    Serial.print(distance);
    Serial.println(" meters");
  } else {
    Serial.println("JSON Parsing Error");
  }
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
}
