#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <DHT.h>

// Wi-Fi Credentials
const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

// Web Server on port 80
ESP8266WebServer server(80);

// Pin Definitions
#define BLUE_LED_PIN 14  // D5
#define YLW_LED_PIN 12   // D6
#define DHT_PIN 4        // D2
#define DHTTYPE DHT11

DHT dht(DHT_PIN, DHTTYPE);

Servo door_servo;
Servo fan_servo;

// Global State Variables
bool isLightOn = false;
bool isFanOn = false;

// Auto Temperature Control Settings
const float TEMP_THRESHOLD = 30.0; // Temperature in °C to turn on fan
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_INTERVAL = 3000; // Check sensor every 3 seconds
float currentTemp = 0.0;
float currentHum = 0.0;

// Helper to set fan hardware state
void setFanState(bool turnOn) {
  if (turnOn) {
    fan_servo.write(0); // Continuous rotation / active state
    isFanOn = true;
  } else {
    fan_servo.write(90); // Stop state for continuous servo
    isFanOn = false;
  }
}

// Helper to open/close gate smoothly
void gateAction(bool openState) {
  if (openState) {
    for (int i = 0; i <= 180; i++) {
      door_servo.write(i);
      delay(10);
    }
  } else {
    for (int i = 180; i >= 0; i--) {
      door_servo.write(i);
      delay(10);
    }
  }
}

// Helper to construct a unified JSON state response
String getSystemStatusJson(float temp = -999.0, float hum = -999.0) {
  String json = "{";
  json += "\"light_status\":\"" + String(isLightOn ? "ON" : "OFF") + "\",";
  json += "\"fan_status\":\"" + String(isFanOn ? "ON" : "OFF") + "\",";
  json += "\"temp_threshold\":" + String(TEMP_THRESHOLD);
  
  // Include temperature and humidity if valid values were provided
  if (temp != -999.0 && hum != -999.0) {
    json += ",\"temperature\":" + String(temp);
    json += ",\"humidity\":" + String(hum);
  }
  
  json += "}";
  return json;
}

// Automatic Temperature Monitoring Logic
void checkTemperatureAutoControl() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Warning: Failed to read from DHT sensor!");
    return;
  }

  currentTemp = temp;
  currentHum = hum;

  // Auto ON: Temperature rises above threshold
  if (currentTemp >= TEMP_THRESHOLD && !isFanOn) {
    Serial.println("[AUTO CONTROL] Temp (" + String(currentTemp) + "°C) >= Threshold (" + String(TEMP_THRESHOLD) + "°C). Turning Fan ON.");
    setFanState(true);
  } 
  // Auto OFF: Temperature drops below threshold with 1.0°C hysteresis buffer
  else if (currentTemp < (TEMP_THRESHOLD - 1.0) && isFanOn) {
    Serial.println("[AUTO CONTROL] Temp (" + String(currentTemp) + "°C) cooled below Threshold. Turning Fan OFF.");
    setFanState(false);
  }
}

// Endpoint Handlers
void handleGateOpen() {
  Serial.println("Received command: Open Gate");
  gateAction(true);
  
  // Auto-close gate after 5 seconds
  delay(5000);
  gateAction(false);
  
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Gate opened and closed\"}");
}

void handleTurnOnLight() {
  Serial.println("Received command: Light ON");
  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(YLW_LED_PIN, LOW);
  isLightOn = true;
  
  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void handleTurnOffLight() {
  Serial.println("Received command: Light OFF");
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(YLW_LED_PIN, HIGH);
  isLightOn = false;
  
  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void handleFanControl() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      setFanState(true);
      Serial.println("Received command: Fan ON (Manual Override)");
      server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
    } else if (state == "off") {
      setFanState(false);
      Serial.println("Received command: Fan OFF (Manual Override)");
      server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid state parameter. Use 'on' or 'off'.\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'state' query parameter (e.g. ?state=on)\"}");
  }
}

void handleSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to read from DHT sensor\"}");
    return;
  }

  currentTemp = temp;
  currentHum = hum;

  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void handleGetStatus() {
  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void setup() {
  Serial.begin(115200);

  // Initialize Pins & Servos
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(YLW_LED_PIN, OUTPUT);
  
  // Default light state (OFF)
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(YLW_LED_PIN, HIGH);
  
  door_servo.attach(D7);
  fan_servo.attach(D8);
  
  // Default fan state (OFF)
  setFanState(false);

  dht.begin();

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSecond ESP8266 Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Define Web Server Routes
  server.on("/gate_open", handleGateOpen);
  server.on("/turn_on_light", handleTurnOnLight);
  server.on("/turn_off_light", handleTurnOffLight);
  server.on("/fan", handleFanControl);
  server.on("/sensors", handleSensorData);
  server.on("/status", handleGetStatus);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Non-blocking timer to periodically read DHT11 and manage automatic fan state
  if (millis() - lastSensorReadTime >= SENSOR_INTERVAL) {
    lastSensorReadTime = millis();
    checkTemperatureAutoControl();
  }
}