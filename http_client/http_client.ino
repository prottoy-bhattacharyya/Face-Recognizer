#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "ArduinoJson.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Servo.h>

Servo door_servo;
Servo fan_servo;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define BLUE_LED_PIN 14
#define YLW_LED_PIN 12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

String server_url = "http://192.168.1.210:8000/";
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


// Helper function to render 2 lines of text on the 128x32 OLED display
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
      message = doc["description"].as<String>();
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
    if(checkRequest("Verify", payload)){
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
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  showText("System Init", "Starting...");
}

void setup() {
  Serial.begin(115200);
  init_display();
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(YLW_LED_PIN, OUTPUT);
  door_servo.attach(D5);
  fan_servo.attach(D6);

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
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    verify_face();
  } else {
    showText("WiFi Disconnected", "Reconnecting...");
  }

  delay(5000);
}