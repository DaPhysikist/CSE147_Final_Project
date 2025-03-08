#include <Arduino.h>
#include <WiFi.h>
#include <HardwareSerial.h>

#define TAPO_DEBUG_MODE // Comment this line to disable debug messages
#include "tapo_device.h"

HardwareSerial mySerial(2);
TapoDevice tapo; 

char buffer[50];
int bufferIndex = 0;
const int NUM_READINGS = 5;
int distanceBuffer[NUM_READINGS] = {0};
int bufferPos = 0;
int totalDistances = 0;
bool bufferFilled = false;

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 16, 17);

  WiFi.begin("mirkwood", "eyeofsauron");
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
    
    tapo.begin("192.168.86.185", "pvmehta936@gmail.com", "Password123");
  } else {
    Serial.println("\nWiFi connection failed!");
  }
  
  Serial.println("ESP32 UART Distance Receiver Started");
  
  for(int i = 0; i < NUM_READINGS; i++) {
    distanceBuffer[i] = 0;
  }
}

int getAverageDistance() {
  if(!bufferFilled && bufferPos == 0) {
    return 0;
  }
  
  int numReadings = bufferFilled ? NUM_READINGS : bufferPos;
  return (numReadings > 0) ? (totalDistances / numReadings) : 0;
}

void addDistanceReading(int newDistance) {
  totalDistances -= distanceBuffer[bufferPos];
  
  distanceBuffer[bufferPos] = newDistance;
  totalDistances += newDistance;
  bufferPos = (bufferPos + 1) % NUM_READINGS;
  
  if(bufferPos == 0) {
    bufferFilled = true;
  }
}

void loop() {
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
        if(currentDistance >= 0) {
          addDistanceReading(currentDistance);
          int avgDistance = getAverageDistance();
          
          Serial.print("Current distance: ");
          Serial.print(currentDistance);
          Serial.print(" mm, Average distance (last ");
          Serial.print(bufferFilled ? NUM_READINGS : bufferPos);
          Serial.print(" readings): ");
          Serial.print(avgDistance);
          Serial.println(" mm");
          
          if (avgDistance > 3000 || avgDistance == 0) {
            tapo.off();
            Serial.println("Tapo turned OFF");
          } else {
            tapo.on();
            Serial.println("Tapo turned ON");
          }
        } else {
          Serial.println("Received invalid distance reading");
        }
      }
      
      bufferIndex = 0;
      buffer[0] = '\0';
    }
  }
  
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.begin("mirkwood", "eyeofsauron");
  }
}