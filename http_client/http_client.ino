#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Arduino JSON by Arduino
#include "ArduinoJson.h"

#include <SPI.h>
#include <Wire.h>

// Adafruit GFX Lib by adafruit
#include <Adafruit_GFX.h>
// Adafruit SSD1306 by adafruit
#include <Adafruit_SSD1306.h>

// i2c keypad by rob tillart
#include <Keypad_I2C.h>

#define I2C_ADDRESS 0x27
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

const String server_url = "http://192.168.1.138:8000/";
const String second_esp_url = "http://192.168.1.55/";

const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {0, 1, 2, 3}; 
byte colPins[COLS] = {4, 5, 6, 7}; 

Keypad_I2C keypad = Keypad_I2C(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2C_ADDRESS);

String inputGatePassword = "";
const String actualGatePassword = "1234";

void showText(const String& line1, const String& line2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  if (line2.length() > 0) {
    display.setCursor(0, 16);
    display.println(line2);
  }
  display.display();
}

bool checkRequest(const String& action, const String& payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("JSON parsing failed: "));
    Serial.println(error.c_str());
    showText(action + " Failed", "JSON Parse Error");
    return false;
  }

  String status = doc["status"].as<String>();

  if (status == "success") {
    String message = doc.containsKey("name") ? doc["name"].as<String>() : 
                    (doc.containsKey("message") ? doc["message"].as<String>() : "Success");
    Serial.println("[" + action + "] OK: " + message);
    showText(action + ": OK", message);
    return true;
  }

  String errorMsg = doc.containsKey("description") ? doc["description"].as<String>() :
                    (doc.containsKey("message") ? doc["message"].as<String>() : "Unknown error");
  Serial.println("[" + action + "] Error: " + errorMsg);
  showText(action + ": FAILED", errorMsg);
  return false;
}

void triggerSecondESP() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient clientSecond;
  HTTPClient httpSecond;
  
  httpSecond.begin(clientSecond, second_esp_url + "gate_open");
  int httpCode = httpSecond.GET();
  
  if (httpCode > 0) {
    String payload = httpSecond.getString();
    if (checkRequest("Gate Open", payload)) {
      delay(2000);
    } else {
      delay(2000);
    }
  } else {
    Serial.println("Controller Error: " + String(httpCode));
    showText("Controller Error", "Code: " + String(httpCode));
    delay(2000);
  }
  httpSecond.end();
}

bool verify_face() {
  showText("Verifying...", "Scanning Face");
  
  WiFiClient clientMain;
  HTTPClient http;
  
  http.begin(clientMain, server_url + "verify/");
  int httpCode = http.GET();
  bool success = false;

  if (httpCode > 0) {
    String payload = http.getString();
    success = checkRequest("Verify", payload);
  } else {
    showText("Verify Error", "HTTP Code: " + String(httpCode));
  }
  
  http.end();
  return success;
}

void updateIdleScreen() {
  String maskedPIN = "";
  for (size_t i = 0; i < inputGatePassword.length(); i++) {
    maskedPIN += "*";
  }
  showText("[A] Scan Face", "PIN: " + maskedPIN);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5);
  keypad.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  showText("System Init", "Starting...");

  WiFi.begin(ssid, password);
  showText("Connecting WiFi", ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  showText("WiFi Connected!", WiFi.localIP().toString());
  delay(1500);

  updateIdleScreen();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    showText("WiFi Disconnected", "Reconnecting...");
    delay(1000);
    return;
  }

  char key = keypad.getKey();

  if (key) {
    if (key == 'A') {
      // Manual trigger for Face Scan
      if (verify_face()) {
        delay(1000);
        triggerSecondESP();
      } else {
        delay(2000);
      }
      inputGatePassword = "";
      updateIdleScreen();
    } 
    else if (key == 'C') {
      // Clear PIN input
      inputGatePassword = "";
      updateIdleScreen();
    } 
    else if (key == '#' || inputGatePassword.length() == 4) {
      // Submit or Auto-Submit on 4 digits
      if (inputGatePassword == actualGatePassword) {
        showText("Password OK", "Opening Gate...");
        delay(1000);
        triggerSecondESP();
      } else {
        showText("Access Denied", "Wrong Password");
        delay(2000);
      }
      inputGatePassword = "";
      updateIdleScreen();
    } 
    else if (key >= '0' && key <= '9') {
      // Append digit
      if (inputGatePassword.length() < 4) {
        inputGatePassword += key;
        updateIdleScreen();

        // Check if 4th digit was just entered
        if (inputGatePassword.length() == 4) {
          delay(200);
          if (inputGatePassword == actualGatePassword) {
            showText("Password OK", "Opening Gate...");
            delay(1000);
            triggerSecondESP();
          } else {
            showText("Access Denied", "Wrong Password");
            delay(2000);
          }
          inputGatePassword = "";
          updateIdleScreen();
        }
      }
    }
  }
}