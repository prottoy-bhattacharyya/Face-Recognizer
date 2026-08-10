#include <LiquidCrystal.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "ArduinoJson.h" 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

int show=0;

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

String server_url = "http://192.168.1.138:8000/";

WiFiClient client;
HTTPClient http;

bool checkRequest(String payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return false;
    }

    // Cast directly to String to enable proper string comparison
    String status = doc["status"].as<String>();

    if (status == "success") {
      String name = doc["name"].as<String>();
      Serial.println("Response OK. Name: " + name);
      lcd.setCursor(0, 0);
      lcd.print(name);
      return true;
    }

    if (status == "failed") {
      // Check both 'description' and 'message' keys
      String errorMsg = "";
      if (doc.containsKey("description")) {
        errorMsg = doc["description"].as<String>();
      } else if (doc.containsKey("message")) {
        errorMsg = doc["message"].as<String>();
      } else {
        errorMsg = "Unknown error";
      }

      lcd.setCursor(0, 0);
      lcd.print("Error:");
      lcd.setCursor(0, 1);
      Serial.println("Error: " + errorMsg);
      return false;
    }

    return false;
}

void enroll_face(String name){
    // 2. HTTP POST Request
    http.begin(client, server_url + "faces/" + name + "/");
    http.addHeader("Content-Type", "application/json");
    int httpPostCode = http.POST("{}");

    if (httpPostCode > 0) {
      String payload = http.getString();
      Serial.println("Enroll Response: " + checkRequest(payload));
    }
    http.end();
}

void verify_face(){
    // 1. HTTP GET Request
    http.begin(client, server_url + "verify/");
    int httpGetCode = http.GET();
    if (httpGetCode > 0) {
      String payload = http.getString();
      Serial.println("Verify Response: " + checkRequest(payload));
    }
    else {
      Serial.println(httpGetCode);
    }
    http.end();
}

void delete_face(String name){
    // 3. HTTP DELETE Request
    http.begin(client, server_url + "faces/" + name + "/");
    int httpDeleteCode = http.sendRequest("DELETE");
    if (httpDeleteCode > 0) {
      String payload = http.getString();
      Serial.println("DELETE Response: " + checkRequest(payload));
    }
    http.end();
}

void init_lcd(){
  lcd.init();
  lcd.backlight();
  Serial.println("lcd init");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("IP: " + WiFi.localIP().toString());

  init_lcd();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    enroll_face("Prottoy");
    delay(1000);
    verify_face();
    delete_face("Prottoy");
  }
  
  delay(1000);
}
