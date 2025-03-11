#include <Arduino.h>
#include <WiFi.h>
#include "tapo_device.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define LED_BUILTIN 2
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define ECHO_PIN 17
#define TRIG_PIN 16
#define FLAME_PIN 23

#define ENCODER_CLK_PIN 5
#define ENCODER_DT_PIN 18
#define ENCODER_SW_PIN 19

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define TAPO_DEBUG_MODE     // Comment this line to disable debug messages
// #define TIMEOUT_PERIOD 15000 // 15 000 mill  = 15 seconds
// #define DISTANCE_THRESHOLD 10.0 // 10 cm
TapoDevice tapo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// adjustable by user
int TIMEOUT_PERIOD = 15000;
int DISTANCE_THRESHOLD = 10.0;

// ultrasonic
float timing = 0.0;
float distance = 0.0;

// time
long int currTime = 0;
long int elapsed = 0;
int timeout = 0;

// flame
int flame = 0;

// Tapo responses
String energy = "";
String power = "";
String energy_usage_day = "";
String powerStr = "";
String energyDayStr = "";
int result_today_runtime = 0;
int result_month_runtime = 0;
int result_today_energy = 0;
int result_month_energy = 0;
int result_electricity_charge_0 = 0;
int result_electricity_charge_1 = 0;
int result_electricity_charge_2 = 0;
const char *result_local_time = "";
int result_current_power = 0;
int tapoState;
int error_code = 0;

// Rotary encoder adjustments
int adjustMode = 0;
int adjustOption = 0; // 0 = timeout, 1 = distance
unsigned long lastAdjustTime = 0;
int encoderState = 0;
unsigned long lastEncoderTime = 0;
unsigned long encoderDelay = 50;

// encoder button
int buttonState = 0; // pullup?
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 500;

int readEncoder()
{
  if ((millis() - lastEncoderTime) < encoderDelay)
  {
    return 0;
  }
  lastEncoderTime = millis();
  lastButtonPress = millis(); // twisting encoder can also be registered as a button press
  // static int lastCLK = digitalRead(ENCODER_CLK_PIN);
  int newCLK = digitalRead(ENCODER_CLK_PIN);
  int newDT = digitalRead(ENCODER_DT_PIN);
  int prevState = encoderState & 0b11;
  encoderState = (encoderState << 2) | (newDT << 1) | newCLK;
  int currState = encoderState & 0b11;
  encoderState = encoderState & 0b1111;
  Serial.print("Encoder State: ");
  Serial.print(encoderState, BIN);
  // Serial.print((encoderState & 0b1000) == 1);
  // Serial.print((encoderState & 0b0100) == 1);
  // Serial.print((encoderState & 0b0010) == 1);
  // Serial.println((encoderState & 0b0001) == 1);

  if (encoderState == 0b1011 | encoderState == 0b1101 | encoderState == 0b0100 | encoderState == 0b0010)
  {
    Serial.println("Encoder State: Counter Clockwise");
    return -1;
  }
  else if (encoderState == 0b1000 | encoderState == 0b0001 | encoderState == 0b0111 | encoderState == 0b1110)
  {
    Serial.println("Encoder State: Clockwise");
    return 1;
  }
  else
  {
    // encoderState = encoderState >> 2; //reset the last two bits
  }
  Serial.println("Encoder State: No Movement");
  return 0; // invalid state transistion, report no movement
}

void encoderButton()
{
  int reading = digitalRead(ENCODER_SW_PIN);

  if ((millis() - lastButtonPress) > debounceDelay)
  {
    // Serial.println("Button Pressed after debounce");
    // Serial.print("Button Reading: ");
    // Serial.print(reading);
    // Serial.print(" Button State: ");
    // Serial.println(buttonState);
    if (reading == 0 && buttonState == 1)
    { // on push down
      // buttonState = reading;
      // Serial.println("Button Pressed");
      if (adjustMode == 0)
      { // enter adjust mode
        // Serial.println("Adjust Mode");
        adjustMode = 1;
        lastAdjustTime = millis();
        adjustOption = 0; // first state
      }
      else
      {
        // Serial.println("Adjust Mode Increment");
        adjustOption = (adjustOption + 1) % 2; // toggle between adjustments once in adjust mode
        lastAdjustTime = millis();
      }
    }
  }
  if (reading != buttonState)
  {
    lastButtonPress = millis();
    // Serial.println("Button Pressed");
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
        TIMEOUT_PERIOD += encoderVal * 1000;
        // cap between 5 and 60 seconds
        if (TIMEOUT_PERIOD < 5000)
        {
          TIMEOUT_PERIOD = 5000;
        }
        else if (TIMEOUT_PERIOD > 60000)
        {
          TIMEOUT_PERIOD = 60000;
        }
      }
      else
      {
        // cap between 1 and 100
        DISTANCE_THRESHOLD += encoderVal * 1;
        if (DISTANCE_THRESHOLD < 1.0)
        {
          DISTANCE_THRESHOLD = 1.0;
        }
        else if (DISTANCE_THRESHOLD > 100.0)
        {
          DISTANCE_THRESHOLD = 100.0;
        }
      }
      lastAdjustTime = millis();
    }
    if ((millis() - lastAdjustTime) > 5000)
    {
      adjustMode = 0;
    }
  }
}

void check_distance_and_shutoff()
{
  if (distance > DISTANCE_THRESHOLD)
  {
    if (timeout != 1)
    { // timeout starts now!
      elapsed = currTime;
      timeout = 1;
    }
    else if (timeout == 1)
    {
      Serial.print("elapsed - currTime:");
      Serial.println(currTime - elapsed);
      if (currTime - elapsed > TIMEOUT_PERIOD && tapoState == 1)
      {
        tapo.off();
        tapo.update();
        Serial.println("Tapo Off");
        if (tapo.getPriority() == 1)
        {
          tapoState = 0;
        }
      }
    }
  }
  else
  {
    if (timeout == 1)
    {
      timeout = 0; // reset timeout
      tapo.on();
      tapo.update();
      Serial.println("Tapo On");
      tapoState = 1;
    }
  }
}

void checkFire()
{
  flame = digitalRead(FLAME_PIN);
  if (flame == 0)
  {
    return;
  }
  if (flame == 1)
  {
    display.clearDisplay();
    display.setTextSize(1); // Smaller text to fit all info
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("FIRE DETECTED!!!");
    display.println("Tapo Off for Safety");
    display.display();
    tapo.off();
    while (tapo.update() == 2)
    {
      // wait for tapo to turn off
      tapo.update();
    }
    // delay(1000);
    //  tapoState = 0;  // tapoState stays the same, we'll revert back to it when the fire is done.
    while (flame == 1)
    {
      flame = digitalRead(FLAME_PIN);
      if (flame == 0 && tapoState == 1) // if the fire is done and the tapo was on, turn it back on
      {
        tapo.on();
        while (tapo.update() == 2)
        {
          // wait for tapo to turn off
          tapo.update();
        }
        // delay(1000);
        tapoState = 1;
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ENCODER_CLK_PIN, INPUT);
  pinMode(ENCODER_DT_PIN, INPUT);
  pinMode(ENCODER_SW_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Connect to WiFi
  WiFi.begin("RESNET-GUEST-DEVICE", "ResnetConnect");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Initialize Tapo device
  tapo.begin("100.117.17.94", "aaryatopiwala@gmail.com", "TapoPassword123");
  tapoState = 1;

  tapo.on();
  // Example: tapo.begin("192.168.1.100", "abc@example.com", "abc123");

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop()
{
  currTime = millis();

  checkFire(); // if fire is detected, turn off tapo
  // ultrasonic pulses
  digitalWrite(TRIG_PIN, LOW);
  delay(2);
  digitalWrite(TRIG_PIN, HIGH);
  delay(10);
  digitalWrite(TRIG_PIN, LOW);

  // tapo.update(); //updates everything in tapo_device.h
  // Serial.println("Tapo Updated");
  timing = pulseIn(ECHO_PIN, HIGH);
  distance = (timing * 0.034) / 2;

  encoderButton();
  adjustSettings();
  check_distance_and_shutoff();

  // Serial.print("Tapo Energy: ");
  tapo.energy();
  tapo.update();

  Serial.println(tapo.getResponse());
  if (tapo.getPriority() == 0 && tapo.gotResponse())
  {                              // if executing another request, don't update the values (otherwise they go to zero if on/off request is occurring.)
    energy = tapo.getResponse(); // only update energy json if priority is 0

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, energy);

    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    JsonObject result = doc["result"];
    result_today_runtime = result["today_runtime"]; // 1064
    result_month_runtime = result["month_runtime"]; // 5382
    result_today_energy = result["today_energy"];   // 72
    result_month_energy = result["month_energy"];   // 531
    result_local_time = result["local_time"];       // "2025-03-04 17:44:35"

    JsonArray result_electricity_charge = result["electricity_charge"];
    result_electricity_charge_0 = result_electricity_charge[0]; // 0
    result_electricity_charge_1 = result_electricity_charge[1]; // 335
    result_electricity_charge_2 = result_electricity_charge[2]; // 231

    result_current_power = result["current_power"]; // 19294

    error_code = doc["error_code"]; // 0
  }

  // display.display();
  display.clearDisplay();
  display.setTextSize(1); // Smaller text to fit all info
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // if in adjustment mode and 5 seconds hasnt passed, show the settings
  if (adjustMode && (millis() - lastAdjustTime) < 5000)
  {
    display.print("Adjusting: ");
    if (adjustOption == 0)
    {
      display.print("Timeout (seconds): ");
      display.println(TIMEOUT_PERIOD / 1000); // milliseconds to seconds
    }
    else
    {
      display.print("Distance (centimeter): ");
      display.println(DISTANCE_THRESHOLD);
    }
  }
  else
  {
    adjustMode = 0;
    display.print("Distance (cm): ");
    display.println(distance);
    display.print("Power (mW): ");
    display.println(result_current_power);
    display.print("Energy Today (Wh): ");
    display.println(result_today_energy);
    display.print("Timeout Left: ");
    display.println((TIMEOUT_PERIOD - (currTime - elapsed)) / 1000);
  }

  display.display();
}
