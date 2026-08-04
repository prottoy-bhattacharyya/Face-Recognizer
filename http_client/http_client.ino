#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> // Include ArduinoJson library

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

String server_url = "http://192.168.1.138:8000/";

WiFiClient client;
HTTPClient http;

void convertJson(){
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error){
      Serial.print("Enroll Face: Enroll JSON parsing failed: ");
      Serial.println(error.c_str());
      return;
    }

    const char* status = doc["status"];
    const char* enrolled_name = doc["name"];
    
    if (status == "success"){
      Serial.println(enrolled_name + "added.")
    }
}

void enroll_face(String name){
    // 2. HTTP POST Request
    http.begin(client, server_url + "faces/" + name + "/");
    http.addHeader("Content-Type", "application/json");
    int httpPostCode = http.POST("{}");

    if (httpPostCode > 0) {
      String payload = http.getString();
      Serial.println("POST Response: " + payload);

    }
    http.end();
}

void verify_face(){
    // 1. HTTP GET Request
    http.begin(client, server_url + "verify/");
    int httpGetCode = http.GET();
    if (httpGetCode > 0) {
      String payload = http.getString();
      Serial.println("GET Response: " + payload);
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
      Serial.println("DELETE Response: " + payload);
    }
    http.end();
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
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    enroll_face("Prottoy");
    delay(1000);
    verify_face();
    delete_face("Prottoy");
  }
  
  delay(1000); // Repeat every 10 seconds
}
