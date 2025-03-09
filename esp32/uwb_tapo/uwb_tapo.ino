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

// Buffer for receiving distance data
char buffer[50];
int bufferIndex = 0;

const String applianceName = "microwave";

// Circular buffer for averaging distance readings
const int NUM_READINGS = 5;
int distanceBuffer[NUM_READINGS] = {0};
int bufferPos = 0;
int totalDistances = 0;
bool bufferFilled = false;

// Timers for periodic data updates
unsigned long lastPeriodicUpdate = 0;
unsigned long lastHistoricalUpdate = 0;
const unsigned long PERIODIC_UPDATE_INTERVAL = 5000;   // 5 sec
const unsigned long HISTORICAL_UPDATE_INTERVAL = 30000; // 30 sec

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
        tapo.begin("192.168.8.9", "pvmehta936@gmail.com", "Password123");
    } else {
        Serial.println("\nWiFi connection failed!");
    }
  
    Serial.println("ESP32 UART Distance Receiver Started");
  
    for(int i = 0; i < NUM_READINGS; i++) {
        distanceBuffer[i] = 0;
    }
}

int getAverageDistance() {
    if (!bufferFilled && bufferPos == 0) return 0;
    int numReadings = bufferFilled ? NUM_READINGS : bufferPos;
    return (numReadings > 0) ? (totalDistances / numReadings) : 0;
}

void addDistanceReading(int newDistance) {
    totalDistances -= distanceBuffer[bufferPos]; // Remove oldest reading
    distanceBuffer[bufferPos] = newDistance;     // Add new reading
    totalDistances += newDistance;               // Update sum

    bufferPos = (bufferPos + 1) % NUM_READINGS;  // Update position

    if (bufferPos == 0) {
        bufferFilled = true;
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

    HTTPClient http;
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

void sendPeriodicData(int distance, String localTime, int currentPower) {
    StaticJsonDocument<256> periodicData;
    periodicData["appliance_name"] = applianceName;
    periodicData["local_time"] = localTime;
    periodicData["current_power"] = currentPower;
    periodicData["distance_ultrasonic"] = 0;
    periodicData["distance_bluetooth"] = 0;
    periodicData["distance_ultrawideband"] = distance;
    periodicData["user_presence_detected"] = (distance < 3000);

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
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
        
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            Serial.print(".");
            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nReconnected to WiFi!");
        } else {
            Serial.println("\nFailed to reconnect.");
        }
    }
}

void loop() {
  checkWiFiConnection();

    while (mySerial.available()) {
        char receivedChar = mySerial.read();
        Serial.print(receivedChar);

        if (bufferIndex < sizeof(buffer) - 1) {
            buffer[bufferIndex++] = receivedChar;
            buffer[bufferIndex] = '\0';
        }

        if (receivedChar == '\n') {
            int currentDistance = 0;
            if (sscanf(buffer, "Distance is %d mm!", &currentDistance) == 1) {
                if (currentDistance >= 0) {
                    addDistanceReading(currentDistance);
                    int avgDistance = getAverageDistance();

                    Serial.print("Current distance: ");
                    Serial.print(currentDistance);
                    Serial.print(" mm, Average distance: ");
                    Serial.print(avgDistance);
                    Serial.println(" mm");

                    // Get Tapo Energy Data
                    String energyData = tapo.energy();
                    int todayRuntime, monthRuntime, todayEnergy, monthEnergy, currentPower;
                    String localTime;

                    if (parseTapoEnergy(energyData, todayRuntime, monthRuntime, todayEnergy, monthEnergy, localTime, currentPower)) {
                        Serial.println("Parsed Tapo energy data successfully.");

                        // Send periodic data every 5 seconds
                        if (millis() - lastPeriodicUpdate > PERIODIC_UPDATE_INTERVAL) {
                            sendPeriodicData(currentDistance, localTime, currentPower);
                            lastPeriodicUpdate = millis();
                        }

                        // Send historical data every 30 seconds
                        if (millis() - lastHistoricalUpdate > HISTORICAL_UPDATE_INTERVAL) {
                            sendHistoricalData(todayRuntime, monthRuntime, todayEnergy, monthEnergy, localTime);
                            lastHistoricalUpdate = millis();
                        }
                    } else {
                        Serial.println("Failed to parse Tapo energy data.");
                    }
                } else {
                    Serial.println("Received invalid distance reading");
                }
            }

            bufferIndex = 0;
            buffer[0] = '\0';
        }
    }
}