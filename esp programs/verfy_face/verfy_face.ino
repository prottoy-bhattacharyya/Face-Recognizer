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

char keymap[19] = "123A456B789C*0#DNF";

I2CKeyPad keypad(KEYPAD_ADDRESS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

String CORRECT_PIN = "1234";
String inputPassword = "";
String sessionCookie = ""; // Stores Django sessionid cookie

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

void login() {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(client, server_url + "login/");
    http.addHeader("Content-Type", "application/json");

    // Tell HTTPClient to collect Set-Cookie from server headers
    const char* headerKeys[] = {"Set-Cookie"};
    http.collectHeaders(headerKeys, 1);

    String loginPayload = "{\"username\": \"admin\", \"password\": \"admin123\"}";
    int httpCode = http.POST(loginPayload);

    if (httpCode > 0) {
      if (http.hasHeader("Set-Cookie")) {
        sessionCookie = http.header("Set-Cookie");
        Serial.println("Session cookie saved successfully!");
      }
      Serial.println("Login Response Code: " + String(httpCode));
    } else {
      Serial.println("Login failed: " + http.errorToString(httpCode));
    }
    http.end();
  }
}

void syncPassword() {
  if (WiFi.status() == WL_CONNECTED) {
    // Define target username
    String targetUser = "admin"; 

    // Append query parameter to the URL
    String url = server_url + "get_updated_password/?username=" + targetUser;
    http.begin(client, url);

    // Pass session cookie if Django session authentication is required
    if (sessionCookie.length() > 0) {
      http.addHeader("Cookie", sessionCookie);
    }

    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      JsonDocument doc;
      
      if (deserializeJson(doc, payload) == DeserializationError::Ok) {
        if (doc["status"] == "success") {
          CORRECT_PIN = doc["password"].as<String>();
          Serial.println("Updated local PIN to: " + CORRECT_PIN);
        }
      }
    } else {
      Serial.println("syncPassword HTTP Error Code: " + String(httpCode));
    }
    http.end();
  }
}

void enroll_face(String name) {
  showText("Enrolling...", name);
  http.begin(client, server_url + "faces/" + name + "/");
  http.addHeader("Content-Type", "application/json");
  if (sessionCookie.length() > 0) http.addHeader("Cookie", sessionCookie);

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
  if (sessionCookie.length() > 0) http.addHeader("Cookie", sessionCookie);

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

  WiFi.mode(WIFI_OFF);
  delay(100);

  Wire.begin(4, 5); // SDA = D2, SCL = D1
  Wire.setClock(100000);
  delay(100);

  Wire.beginTransmission(KEYPAD_ADDRESS);
  Wire.write(0xFF);
  Wire.endTransmission();
  delay(100);

  init_keypad();
  init_display();
  Wire.setClock(100000);

  delay(2000);

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
  delay(1000);
  
  // Authenticate and fetch initial PIN state
  login();
  syncPassword();
  
  showText("Ready", "PIN or Press A");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (keypad.isPressed()) {
      char key = keypad.getChar();

      while (keypad.isPressed()) {
        yield();
      }

      if (key == 'A') {
        inputPassword = "";
        verify_face();
        delay(2000);
        showText("Ready", "PIN or Press A");
      } 
      else if (key == '#') {
        syncPassword(); // Fetch latest password state from Django
        
        if (inputPassword.length() > 0) {
          if (inputPassword == CORRECT_PIN) {
            showText("Access Granted", "Opening Gate...");
            triggerSecondESP();
            delay(2000);
          } else {
            showText("Access Denied", "Wrong Password!");
            delay(2000);
          }
          inputPassword = "";
          showText("Ready", "PIN or Press A");
        }
      } 
      else if (key == 'C') {
        inputPassword = "";
        showText("PIN Cleared", "");
        delay(1000);
        showText("Ready", "PIN or Press A");
      } 
      else if (key != 'N' && key != 'F') {
        if (inputPassword.length() < 8) {
          inputPassword += key;

          String maskedPin = "";
          for (size_t i = 0; i < inputPassword.length(); i++) {
            maskedPin += "* ";
          }
          showText("Enter PIN:", maskedPin);
        }
      }
    }
  } else {
    showText("WiFi Disconnected", "Reconnecting...");
    delay(1000);
  }
}