#include <HardwareSerial.h>

HardwareSerial mySerial(2);  // Use UART2

void setup() {
  // Start the default Serial Monitor (for debugging)
  Serial.begin(115200);

  // Initialize UART2 with TX = GPIO17, RX = GPIO16
  mySerial.begin(115200, SERIAL_8N1, 16, 17);
}

void loop() {
  // Check if data is available in UART2
  while (mySerial.available()) {
    char receivedChar = mySerial.read();  // Read a character
    Serial.print(receivedChar);           // Print it to the Serial Monitor
  }
}
