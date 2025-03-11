#include <Arduino.h>
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

HardwareSerial mySerial(2);
TapoDevice tapo;

// WiFi Credentials
const char *ssid = "DaWiFi";
const char *password = "DaPassword123!";

// FastAPI Server Address
const char *serverIP = "192.168.8.27";
const int serverPort = 6543;

// Buffer for receiving distance data
char buffer[50];
int bufferIndex = 0;

const String applianceName = "monitor";

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
const unsigned long HISTORICAL_UPDATE_INTERVAL = 30000; // 30 sec

// ultrasonic
#define ECHO_PIN 17
#define TRIG_PIN 16

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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Main control variables
int TIMEOUT_PERIOD = 15000;  // in ms
int DISTANCE_THRESHOLD = 10; // in centimeters
//int avgDistance = 0;

// rotary encoder / timeout handling
int adjustMode = 0;
int adjustOption = 0; // 0 = timeout, 1 = distance threshold
unsigned long lastAdjustTime = 0;

// ultrasonic
float timing = 0.0;
float distance = 0.0;

int encoderState = 0;
unsigned long lastEncoderTime = 0;
unsigned long encoderDelay = 50;

int buttonState = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 750;

// tapo off timeout control
long int currTime = 0;
long int elapsed = 0;
int timeout = 0;
int tapoState = 1;

// energy data
int result_today_runtime = 0, result_month_runtime = 0;
int result_today_energy = 0, result_month_energy = 0;
int result_current_power = 0;
String result_local_time = "";

int readEncoder()
{
    if ((millis() - lastEncoderTime) < encoderDelay)
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
                DISTANCE_THRESHOLD += encoderVal * 5;
                if (DISTANCE_THRESHOLD < 20)
                    DISTANCE_THRESHOLD = 20;
                if (DISTANCE_THRESHOLD > 1000)
                    DISTANCE_THRESHOLD = 1000;
            }
            lastAdjustTime = millis();
        }
        if ((millis() - lastAdjustTime) > 5000)
        {
            adjustMode = 0;
        }
    }
}

void checkFire()
{
    int flame = digitalRead(FLAME_PIN);
    if (flame == 0)
        return;
    if (flame == 1)
    {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("FIRE DETECTED!!!");
        display.println("Tapo Off for Safety");
        display.display();
        tapo.off();
        while (tapo.update() == 2)
        {
            tapo.update();
        }
        // Wait until fire is no longer detected
        while (digitalRead(FLAME_PIN) == 1)
        {
            delay(100);
        }
        // Reactivate if needed
        tapo.on();
        while (tapo.update() == 2)
        {
            tapo.update();
        }
    }
}

void check_distance_and_shutoff(int avgDistance)
{
    // Convert avgDistance from mm to cm for comparison
    //float avgDistance_cm = avgDistance / 10.0; // UWB returns millimeter
    float avgDistance_cm = avgDistance; // by default ultrasonic returns centimeter data
    currTime = millis();
    if (avgDistance_cm > DISTANCE_THRESHOLD)
    {
        if (timeout != 1)
        { // starting timeout
            elapsed = currTime;
            timeout = 1;
        }
        else
        {
            if (currTime - elapsed > TIMEOUT_PERIOD && tapoState == 1)
            {
                tapo.off();
                tapo.update();
                Serial.println("Tapo Off due to timeout");
                tapoState = 0;
            }
        }
    }
    else
    {
        elapsed = currTime;
        if (timeout == 1)
        {
            timeout = 0; // reset timeout
            tapo.on();
            tapo.update();
            Serial.println("Tapo On (reset timeout)");
            tapoState = 1;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    mySerial.begin(115200, SERIAL_8N1, 16, 17);

    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000)
    {
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        tapo.begin("192.168.8.18", "pvmehta936@gmail.com", "Password123");
    }
    else
    {
        Serial.println("\nWiFi connection failed!");
    }

    // Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        while (1)
            ;
    }
    display.clearDisplay();
    display.display();

    //  digital pins
    pinMode(FLAME_PIN, INPUT);
    pinMode(ENCODER_CLK_PIN, INPUT);
    pinMode(ENCODER_DT_PIN, INPUT);
    pinMode(ENCODER_SW_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.println("ESP32 UART Distance Receiver Started");

    for (int i = 0; i < NUM_READINGS; i++)
    {
        distanceBuffer[i] = 0;
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

// Function to parse JSON response from `tapo.energy()`
bool parseTapoEnergy(String jsonString, int &todayRuntime, int &monthRuntime, int &todayEnergy, int &monthEnergy, String &localTime, int &currentPower)
{
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, jsonString);
    if (error)
    {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        return false;
    }

    if (doc.containsKey("result"))
    {
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
    periodicData["distance_bluetooth"] = 0;
    periodicData["distance_ultrawideband"] = distance;
    periodicData["user_presence_detected"] = (distance < (DISTANCE_THRESHOLD * 10));

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
}

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
            display.println(DISTANCE_THRESHOLD);
        }
    }
    else
    {
        display.print("Dist, cm: ");
        display.println(distance);
        display.print("Power, mW: ");
        display.println(result_current_power);
        display.print("Energy Today, Wh: ");
        display.println(result_today_energy);
        display.print("Timeout Left, sec: ");
        display.println((TIMEOUT_PERIOD - (currTime - elapsed)) / 1000);
    }
    display.display();
}

void loop()
{

    checkWiFiConnection();
    checkFire();
    // ultrasonic
    digitalWrite(TRIG_PIN, LOW);
    delay(2);
    digitalWrite(TRIG_PIN, HIGH);
    delay(10);
    digitalWrite(TRIG_PIN, LOW);
    timing = pulseIn(ECHO_PIN, HIGH);
    distance = (timing * 0.034) / 2;

    encoderButton();
    adjustSettings();
    check_distance_and_shutoff(distance);
    updateOLED();

    String energyData = tapo.energy();
    tapo.update();
    int todayRuntime, monthRuntime, todayEnergy, monthEnergy, currentPower;
    String localTime;

    if (parseTapoEnergy(energyData, todayRuntime, monthRuntime, todayEnergy, monthEnergy, localTime, currentPower))
    {
        Serial.println("Parsed Tapo energy data successfully.");
        result_today_runtime = todayRuntime;
        result_month_runtime = monthRuntime;
        result_today_energy = todayEnergy;
        result_month_energy = monthEnergy;
        result_current_power = currentPower;
        result_local_time = localTime;

        if (millis() - lastPeriodicUpdate > PERIODIC_UPDATE_INTERVAL)
        {
            sendPeriodicData(distance, localTime, currentPower);
            lastPeriodicUpdate = millis();
        }

        if (millis() - lastHistoricalUpdate > HISTORICAL_UPDATE_INTERVAL)
        {
            sendHistoricalData(todayRuntime, monthRuntime, todayEnergy, monthEnergy, localTime);
            lastHistoricalUpdate = millis();
        }
    }

}
