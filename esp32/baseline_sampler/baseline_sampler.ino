#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define TAPO_DEBUG_MODE // Comment this line to disable debug messages
#include "tapo_device.h"

HardwareSerial mySerial(2);
TapoDevice tapo;

// WiFi Credentials
const char* ssid = "DaWiFi";
const char* password = "DaPassword123!";

// FastAPI Server Address
const char* serverIP = "192.168.8.27";
const int serverPort = 6543;

const String applianceName = "mac-mini";

// Timers for periodic data updates
unsigned long lastPeriodicUpdate = 0;
unsigned long lastHistoricalUpdate = 0;
const unsigned long PERIODIC_UPDATE_INTERVAL = 10000;   // 10 sec
const unsigned long HISTORICAL_UPDATE_INTERVAL = 3600000; // 1 hr

// HTTPClient instance
HTTPClient http;

void setup() {
    Serial.begin(115200);
    mySerial.begin(115200, SERIAL_8N1, 16, 17);

    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
  
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
        delay(1000);
        Serial.print(".");
    }
  
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        tapo.begin("192.168.8.18", "pvmehta936@gmail.com", "Password123");
    } else {
        Serial.println("\nWiFi connection failed!");
    }
}

// Function to parse JSON response from `tapo.energy()`
bool parseTapoEnergy(String jsonString, int &todayRuntime, int &monthRuntime, int &todayEnergy, int &monthEnergy, String &localTime, int &currentPower) {
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        return false;
    }

    if (doc.containsKey("result")) {
        todayRuntime = doc["result"]["today_runtime"];
        monthRuntime = doc["result"]["month_runtime"];
        todayEnergy = doc["result"]["today_energy"];
        monthEnergy = doc["result"]["month_energy"];
        localTime = doc["result"]["local_time"].as<String>();
        currentPower = doc["result"]["current_power"];

        return true;
    }

    return false;
}

// Function to send HTTP POST requests
void sendDataToServer(const char* route, const char* jsonPayload) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Skipping data send.");
        return;
    }

    String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/" + String(route);

    Serial.print("Sending data to: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
        Serial.print("Response Code: ");
        Serial.println(httpResponseCode);
        Serial.println("Response: " + http.getString());
    } else {
        Serial.print("Error sending POST request: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void sendPeriodicData(String localTime, int currentPower) {
    StaticJsonDocument<256> periodicData;
    periodicData["appliance_name"] = applianceName;
    periodicData["local_time"] = localTime;
    periodicData["current_power"] = currentPower;
    periodicData["distance_ultrasonic"] = 0;
    periodicData["distance_bluetooth"] = 0;
    periodicData["distance_ultrawideband"] = 0;
    periodicData["user_presence_detected"] = 1;

    char periodicJson[256];
    serializeJson(periodicData, periodicJson);

    String path = "shuteye_periodic_measurement_data";
    sendDataToServer(path.c_str(), periodicJson);
}

void sendHistoricalData(int todayRuntime, int monthRuntime, int todayEnergy, int monthEnergy, String localTime) {
    StaticJsonDocument<256> historicalData;
    historicalData["appliance_name"] = applianceName;
    historicalData["local_time"] = localTime;
    historicalData["today_runtime"] = todayRuntime;
    historicalData["month_runtime"] = monthRuntime;
    historicalData["today_energy"] = todayEnergy;
    historicalData["month_energy"] = monthEnergy;

    char historicalJson[256];
    serializeJson(historicalData, historicalJson);

    String path = "shuteye_historical_data";
    sendDataToServer(path.c_str(), historicalJson);
}

void checkWiFiConnection() {
    static unsigned long lastReconnectAttempt = 0;
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastReconnectAttempt > 10000) {
            Serial.println("WiFi disconnected. Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
            lastReconnectAttempt = millis();
        }
    }
}

void loop() {
    checkWiFiConnection();

    // Get Tapo Energy Data
    String energyData = tapo.energy();
    int todayRuntime, monthRuntime, todayEnergy, monthEnergy, currentPower;
    String localTime;

    if (parseTapoEnergy(energyData, todayRuntime, monthRuntime, todayEnergy, monthEnergy, localTime, currentPower)) {
        Serial.println("Parsed Tapo energy data successfully.");
        
        if (millis() - lastPeriodicUpdate > PERIODIC_UPDATE_INTERVAL) {
            sendPeriodicData(localTime, currentPower);
            Serial.println("Sent periodic data.");
            lastPeriodicUpdate = millis();
        }

        if (millis() - lastHistoricalUpdate > HISTORICAL_UPDATE_INTERVAL) {
            sendHistoricalData(todayRuntime, monthRuntime, todayEnergy, monthEnergy, localTime);
            Serial.println("Sent historical data.");
            lastHistoricalUpdate = millis();
        }
    }
}
