#include <PubSubClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define TAPO_DEBUG_MODE // Comment this line to disable debug messages
#include "tapo_device.h"

// flame sensor
#define FLAME_PIN 23

// Rotary encoder pins
#define ENCODER_CLK_PIN 5
#define ENCODER_DT_PIN 18
#define ENCODER_SW_PIN 19

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

const String applianceName = "lamp";

// WiFi Credentials
const char *ssid = "DaWiFi";
const char *password = "DaPassword123!";

const char* mqtt_broker = "192.168.8.27";
const int mqtt_port = 1883;
const char* mqtt_topic = "espresense/devices/pranav-phone/living_room";
const char* mqtt_username = "DaUser";
const char* mqtt_password = "DaPassword123!";

// FastAPI Server Address
const char *serverIP = "192.168.8.27";
const int serverPort = 6543;

// Buffer for receiving distance data
char buffer[50];
int bufferIndex = 0;

// Circular buffer for averaging distance readings
const int NUM_READINGS = 5;
int distanceBuffer[NUM_READINGS] = {0};
int bufferPos = 0;
int totalDistances = 0;
bool bufferFilled = false;

// Timers for periodic data updates
unsigned long lastPeriodicUpdate = 0;
unsigned long lastHistoricalUpdate = 0;
const unsigned long PERIODIC_UPDATE_INTERVAL = 5000;    // 5 sec
const unsigned long HISTORICAL_UPDATE_INTERVAL = 3600000; // 1 hr

// Main control variables
int TIMEOUT_PERIOD = 15000;  // in ms
int UWB_DISTANCE_THRESHOLD_CM = 30; // in centimeters
int avgDistance = 0;

// Bluetooth distance
float bluetoothDistance;
bool userPresenceDetected;

// rotary encoder / timeout handling
int adjustMode = 0;
int adjustOption = 0; // 0 = timeout, 1 = distance threshold
unsigned long lastAdjustTime = 0;

// rotary encoder (rotation)
int encoderState = 0;
unsigned long lastEncoderTime = 0;
unsigned long encoderDelay = 25;

// rotary encoder (button)
int buttonState = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 400;

int flame;

// tapo off timeout control
long int currTime = 0;
long int elapsed = 0;
int timeout = 0;
int tapoState = 1;

// energy data
int resultTodayRuntime = 0, resultMonthRuntime = 0;
int resultTodayEnergy = 0, resultMonthEnergy = 0;
int resultCurrentPower = 0;
String resultLocalTime = "";

HardwareSerial mySerial(2);  // UART interface
TapoDevice tapo;
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int readEncoder()
{
    if ((millis() - lastEncoderTime) < encoderDelay) // debouncing
        return 0;
    lastEncoderTime = millis();
    lastButtonPress = millis(); // twisting can be treated as a press
    int newCLK = digitalRead(ENCODER_CLK_PIN);
    int newDT = digitalRead(ENCODER_DT_PIN);
    encoderState = (encoderState << 2) | (newDT << 1) | newCLK;
    encoderState = encoderState & 0b1111;

    // Check for valid state transitions
    if (encoderState == 0b1011 || encoderState == 0b1101 || encoderState == 0b0100 || encoderState == 0b0010)
    {
        Serial.println("Encoder: Counter Clockwise");
        return -1;
    }
    else if (encoderState == 0b1000 || encoderState == 0b0001 || encoderState == 0b0111 || encoderState == 0b1110)
    {
        Serial.println("Encoder: Clockwise");
        return 1;
    }
    Serial.println("Encoder: No Movement");
    return 0;
}

// determines if the button was pressed
// if the button was pressed, it will enter adjust mode
void encoderButton()
{
    int reading = digitalRead(ENCODER_SW_PIN);
    if ((millis() - lastButtonPress) > debounceDelay)
    {
        if (reading == 0 && buttonState == 1)
        { // button pressed
            if (adjustMode == 0)
            {
                adjustMode = 1;
                lastAdjustTime = millis();
                // adjustOption = 0; // first adjustment: timeout period
                adjustOption = (adjustOption + 1) % 2;
            }
            else
            {
                adjustOption = (adjustOption + 1) % 2;
                lastAdjustTime = millis();
            }
        }
    }
    if (reading != buttonState)
    {
        lastButtonPress = millis();
    }
    buttonState = reading;
}

// if in adjust mode, adjust the settings depending on rotary encoder input
void adjustSettings()
{
    if (adjustMode)
    {
        int encoderVal = readEncoder();
        if (encoderVal != 0)
        {
            if (adjustOption == 0)
            {
                TIMEOUT_PERIOD += encoderVal * 5000;
                if (TIMEOUT_PERIOD < 5000)
                    TIMEOUT_PERIOD = 5000;
                if (TIMEOUT_PERIOD > 60000)
                    TIMEOUT_PERIOD = 60000;
            }
            else
            {
                UWB_DISTANCE_THRESHOLD_CM += encoderVal * 5;
                if (UWB_DISTANCE_THRESHOLD_CM < 20)
                    UWB_DISTANCE_THRESHOLD_CM = 20;
                if (UWB_DISTANCE_THRESHOLD_CM > 1000)
                    UWB_DISTANCE_THRESHOLD_CM = 1000;
            }
            lastAdjustTime = millis();
        }
        if ((millis() - lastAdjustTime) > 5000) // timeout of adjusting settings after 5 seconds
        {
            adjustMode = 0;
        }
    }
}

void checkFire()
{
    if (flame == 0)
        return;
    if (flame == 1)
    {
        Serial.println("Fire!");
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("FIRE DETECTED!!!");
        display.println("Tapo Off for Safety");
        display.display();
        while (digitalRead(FLAME_PIN) == 1)
        {
            tapo.off();
            tapo.update();
        }
    }
}

void check_distance_and_shutoff()
{
    // Convert avgDistance from mm to cm for comparison
    float avgDistance_cm = avgDistance / 10.0;
    currTime = millis();
    if (avgDistance_cm < UWB_DISTANCE_THRESHOLD_CM && (bluetoothDistance < 10.0))
    {
      userPresenceDetected = true;
      elapsed = currTime;
      tapoState = 1;
      if (!flame) {
        tapo.on();
        tapo.update();
      }
      if (timeout == 1)
      {
          timeout = 0; // reset timeout
          Serial.println("Tapo On (reset timeout)");
          
      }
    }
    else
    {
      userPresenceDetected = false;
      if (timeout != 1)
      { // starting timeout
          elapsed = currTime;
          timeout = 1;
      }
      else
      {
        // if distance still high and timeout countdown complete, turn off tapo
          if (currTime - elapsed > TIMEOUT_PERIOD && tapoState == 1)
          {
              tapo.off();
              tapo.update();
              Serial.println("Tapo Off due to timeout");
              tapoState = 0;
          } 
      }
    }
}

int getAverageDistance()
{
    if (!bufferFilled && bufferPos == 0)
        return 0;
    int numReadings = bufferFilled ? NUM_READINGS : bufferPos;
    return (numReadings > 0) ? (totalDistances / numReadings) : 0;
}

void addDistanceReading(int newDistance)
{
    totalDistances -= distanceBuffer[bufferPos]; // Remove oldest reading
    distanceBuffer[bufferPos] = newDistance;     // Add new reading
    totalDistances += newDistance;               // Update sum

    bufferPos = (bufferPos + 1) % NUM_READINGS; // Update position

    if (bufferPos == 0)
    {
        bufferFilled = true;
    }
}

// Function to parse JSON response from tapo.energy()
bool parseTapoEnergy(String jsonString)
{
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, jsonString);
    if (error)
    {
        return false;
    }

    if (doc.containsKey("result"))
    {
        resultTodayRuntime = doc["result"]["today_runtime"];
        resultMonthRuntime = doc["result"]["month_runtime"];
        resultTodayEnergy = doc["result"]["today_energy"];
        resultMonthEnergy = doc["result"]["month_energy"];
        resultLocalTime = doc["result"]["local_time"].as<String>();
        resultCurrentPower = doc["result"]["current_power"];

        return true;
    }

    return false;
}

// Function to send HTTP POST requests
void sendDataToServer(const char *route, const char *jsonPayload)
{
    if (WiFi.status() != WL_CONNECTED)
    {
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

    if (httpResponseCode > 0)
    {
        Serial.print("Response Code: ");
        Serial.println(httpResponseCode);
        Serial.println("Response: " + http.getString());
    }
    else
    {
        Serial.print("Error sending POST request: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void sendPeriodicData(int distance, String localTime, int currentPower)
{
    StaticJsonDocument<256> periodicData;
    periodicData["appliance_name"] = applianceName;
    periodicData["local_time"] = localTime;
    periodicData["current_power"] = currentPower;
    periodicData["distance_ultrasonic"] = 0;
    periodicData["distance_bluetooth"] = bluetoothDistance;
    periodicData["distance_ultrawideband"] = avgDistance;
    periodicData["user_presence_detected"] = userPresenceDetected;

    char periodicJson[256];
    serializeJson(periodicData, periodicJson);

    String path = "shuteye_periodic_measurement_data";
    sendDataToServer(path.c_str(), periodicJson);
}

void sendHistoricalData(int todayRuntime, int monthRuntime, int todayEnergy, int monthEnergy, String localTime)
{
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

void checkWiFiConnection()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi disconnected. Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();

        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
        {
            Serial.print(".");
            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nReconnected to WiFi!");
        }
        else
        {
            Serial.println("\nFailed to reconnect.");
        }
    }

    if(!client.connected()){connectToMQTT();}
}

// OLED normally shows: power, today's energy, distance, and the timeout left
// If in adjust mode, it shows the current parameter being adjusted.
void updateOLED()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    if (adjustMode && (millis() - lastAdjustTime) < 5000)
    {
        display.print("Adjusting: ");
        if (adjustOption == 0)
        {
            display.print("Timeout (sec): ");
            display.println(TIMEOUT_PERIOD / 1000);
        }
        else
        {
            display.print("Distance Thresh (cm): ");
            display.println(UWB_DISTANCE_THRESHOLD_CM);
        }
    }
    else
    {
        display.print("UWB Dist, cm: ");
        display.println(avgDistance / 10.0);
        display.print("Power, mW: ");
        display.println(resultCurrentPower);
        display.print("Energy Today, Wh: ");
        display.println(resultTodayEnergy);
        display.print("Timeout Left, sec: ");
        display.println((TIMEOUT_PERIOD - (currTime - elapsed)) / 1000);
    }
    display.display();
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
    bluetoothDistance = doc["distance"];
    Serial.print("Extracted Distance: ");
    Serial.print(bluetoothDistance);
    Serial.println(" meters");
  } else {
    Serial.println("JSON Parsing Error");
  }
}

void uartCallback() {
    // Handle data received on UART
    while (mySerial.available()) {
        char receivedChar = mySerial.read();

        // Handle the incoming data in the buffer
        if (bufferIndex < sizeof(buffer) - 1) {
            buffer[bufferIndex++] = receivedChar;
            buffer[bufferIndex] = '\0';
        }

        // Process when a new line is received
        if (receivedChar == '\n') {
            int currentDistance = 0;
            if (sscanf(buffer, "Distance is %d mm!", &currentDistance) == 1) {
                if (currentDistance >= 0) {
                    addDistanceReading(currentDistance);
                }
            }
            bufferIndex = 0;
            buffer[0] = '\0';
        }
    }
}

void setup() {
    Serial.begin(115200);
    mySerial.begin(115200, SERIAL_8N1, 16, 17);  // Define UART pins (RX=16, TX=17)
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed!");
    }

    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    connectToMQTT();

    tapo.begin("192.168.8.9", "pvmehta936@gmail.com", "Password123");

    // Set UART interrupt callback for data handling
    mySerial.onReceive(uartCallback);

    // Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (1);
    }
    display.clearDisplay();
    display.display();

    // Digital pins setup
    pinMode(FLAME_PIN, INPUT);
    pinMode(ENCODER_CLK_PIN, INPUT);
    pinMode(ENCODER_DT_PIN, INPUT);
    pinMode(ENCODER_SW_PIN, INPUT);

    // Initialize distance buffer
    for (int i = 0; i < NUM_READINGS; i++) {
        distanceBuffer[i] = 0;
    }

    Serial.println("ShutEye Device Started");
}

void loop() {
    flame = digitalRead(FLAME_PIN);
    checkWiFiConnection();
    checkFire();
    encoderButton();
    adjustSettings();
    check_distance_and_shutoff();
    updateOLED();
    // The uartCallback is now handling UART reads.
    avgDistance = getAverageDistance();

    String energyData = tapo.energy();
    tapo.update();

    if (parseTapoEnergy(energyData))
    {
      Serial.println("Parsed Tapo energy data successfully.");

      

      if (millis() - lastPeriodicUpdate > PERIODIC_UPDATE_INTERVAL)
      {
          sendPeriodicData(avgDistance, resultLocalTime, resultCurrentPower);
          lastPeriodicUpdate = millis();
      }

      if (millis() - lastHistoricalUpdate > HISTORICAL_UPDATE_INTERVAL)
      {
          sendHistoricalData(resultTodayRuntime, resultMonthRuntime, resultTodayEnergy, resultMonthEnergy, resultLocalTime);
          lastHistoricalUpdate = millis();
      }
    }
    client.loop();
}