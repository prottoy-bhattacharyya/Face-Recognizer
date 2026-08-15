#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "ArduinoJson.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2CKeyPad.h>

#define KEYPAD_ADDRESS 0x20
#define LED_ADDRESSS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1

// Rob Tillaart's I2CKeyPad map: 16 keys + 'N' (NoKey) + 'F' (Fail) + null terminator
char keymap[19] = "123A456B789C*0#DNF";

I2CKeyPad keypad(KEYPAD_ADDRESS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

String server_url = "http://192.168.1.138:8000/";
String second_esp_url = "http://192.168.1.55/";

WiFiClient client;
HTTPClient http;

void triggerSecondESP() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient httpSecond;
    httpSecond.begin(client, second_esp_url + "gate_open");
    int httpCode = httpSecond.GET();
    if (httpCode > 0) {
      Serial.println("Second ESP8266 responded: " + String(httpCode));
    } else {
      Serial.println("Error communicating with Second ESP: " + String(httpCode));
    }
    httpSecond.end();
  }
}

// Helper function to render 2 lines of text on the OLED display
void showText(String line1, String line2 = "") {
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

bool checkRequest(String action, String payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    showText(action + " Failed", "JSON Parse Error");
    return false;
  }

  String status = doc["status"].as<String>();

  if (status == "success") {
    String message = "";
    if (doc.containsKey("name")) {
      message = doc["name"].as<String>();
    } else if (doc.containsKey("message")) {
      message = doc["message"].as<String>();
    } else {
      message = "Success";
    }

    Serial.println("[" + action + "] OK: " + message);
    showText(action + ": OK", message);
    return true;
  }

  if (status == "failed") {
    String errorMsg = "Unknown error";
    if (doc.containsKey("description")) {
      errorMsg = doc["description"].as<String>();
    }

    Serial.println("[" + action + "] Error: " + errorMsg);
    showText(action + ": FAILED", errorMsg);
    return false;
  }

  return false;
}

void enroll_face(String name) {
  showText("Enrolling...", name);
  
  http.begin(client, server_url + "faces/" + name + "/");
  http.addHeader("Content-Type", "application/json");
  int httpPostCode = http.POST("{}");

  if (httpPostCode > 0) {
    String payload = http.getString();
    checkRequest("Enroll", payload);
  } else {
    showText("Enroll Error", "HTTP Code: " + String(httpPostCode));
  }
  http.end();
}

void verify_face() {
  showText("Verifying...", "Scanning face");
  
  http.begin(client, server_url + "verify/");
  int httpGetCode = http.GET();

  if (httpGetCode > 0) {
    String payload = http.getString();
    if (checkRequest("Verify", payload)) {
      triggerSecondESP();
    }
  } else {
    showText("Verify Error", "HTTP Code: " + String(httpGetCode));
  }
  http.end();
}

void delete_face(String name) {
  showText("Deleting...", name);
  
  http.begin(client, server_url + "faces/" + name + "/");
  int httpDeleteCode = http.sendRequest("DELETE");

  if (httpDeleteCode > 0) {
    String payload = http.getString();
    checkRequest("Delete", payload);
  } else {
    showText("Delete Error", "HTTP Code: " + String(httpDeleteCode));
  }
  http.end();
}

void init_display() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, LED_ADDRESSS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  showText("System Init", "Starting...");
}

void init_keypad(){
  if (!keypad.begin()) {
    Serial.println("ERROR: Could not find I2C Keypad!");
    showText("Keypad Error", "Check 0x20");
  } else {
    Serial.println("SUCCESS: I2C Keypad found at " + String(KEYPAD_ADDRESS));
    keypad.loadKeyMap(keymap);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // 1. Disable WiFi temporarily to prevent current spikes during I2C setup
  WiFi.mode(WIFI_OFF);
  delay(100);
  Serial.println(F("Wifi Off"));


  // 3. Initialize Wire bus
  Wire.begin(4, 5); // SDA = D2, SCL = D1
  Wire.setClock(100000);
  delay(100);


  Wire.beginTransmission(KEYPAD_ADDRESS);
  Wire.write(0xFF);
  Wire.endTransmission();
  delay(100);

  init_keypad();

  init_display();
  Wire.setClock(100000); // Ensure display init didn't force 400kHz

  delay(2000);
  showText("Ready", "Press A to verify");


  delay(1000);

  // 8. Turn WiFi back on and connect
  WiFi.mode(WIFI_STA);
  showText("Connecting WiFi", ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.println("IP: " + WiFi.localIP().toString());
  
  showText("WiFi Connected!", WiFi.localIP().toString());
  delay(2000);
  showText("Ready", "Press A to verify");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (keypad.isPressed()) {
      char key = keypad.getChar();

      // Wait for key release to prevent repeated triggers
      while (keypad.isPressed()) {
        yield();
      }

      if (key == 'A') {
        verify_face();
        delay(2000);
        showText("Ready", "Press A to verify");
      } else if (key != 'N' && key != 'F') {
        showText("Key Pressed:", String(key));
      }
    }
  } else {
    showText("WiFi Disconnected", "Reconnecting...");
    delay(1000);
  }
}