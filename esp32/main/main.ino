#include <Arduino.h>
#include <WiFi.h>

#define TAPO_DEBUG_MODE // Comment this line to disable debug messages
#include "tapo_device.h"

TapoDevice tapo;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin("RESNET-GUEST-DEVICE", "ResnetConnect");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    // Initialize Tapo device
    tapo.begin("100.117.17.94", "aaryatopiwala@gmail.com", "TapoPassword123");
    // Example: tapo.begin("192.168.1.100", "abc@example.com", "abc123");
}

void loop() {
    //tapo.on(); // Turn on the device
    //delay(5000);
    //tapo.off(); // Turn off the device
    Serial.print("Tapo Energy: ");
    Serial.println(tapo.energy()); // get energy
    Serial.print("Tapo Emeter: ");
    Serial.println(tapo.emeter()); // get energy
    delay(2000);
}
