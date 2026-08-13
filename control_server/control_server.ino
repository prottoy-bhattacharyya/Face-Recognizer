#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <DHT.h>

const char* ssid = "Bachelor Family 2.4G";
const char* password = "passwordnai";

ESP8266WebServer server(80);

#define BLUE_LED_PIN 14  // D5
#define YLW_LED_PIN 12   // D6
#define DHT_PIN 4        // D2
#define DHTTYPE DHT11
#define DOOR_SERVO_PIN D7
#define FAN_SERVO_PIN D8

DHT dht(DHT_PIN, DHTTYPE);

Servo door_servo;
Servo fan_servo;

// Global State Variables
bool isLightOn = false;
bool isFanOn = false;
bool isAutoMode = false; // Default to MANUAL mode

// Non-blocking Gate Control Variables
bool isGateOpen = false;
unsigned long gateOpenStartTime = 0;
const unsigned long GATE_OPEN_DURATION = 5000; // 5 seconds open time

// Auto Temperature Control Settings
const float TEMP_THRESHOLD = 30.0;
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_INTERVAL = 3000;
float currentTemp = 0.0;
float currentHum = 0.0;

void setLightState(bool turnOn) {
  if (turnOn) {
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(YLW_LED_PIN, LOW);
    isLightOn = true;
  } else {
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(YLW_LED_PIN, HIGH);
    isLightOn = false;
  }
}

void setFanState(bool turnOn) {
  if (turnOn) {
    fan_servo.write(0);
    isFanOn = true;
  } else {
    fan_servo.write(90);
    isFanOn = false;
  }
}

void setGateState(bool openState) {
  if (openState) {
    door_servo.write(180);
    isGateOpen = true;
    gateOpenStartTime = millis();
  } else {
    door_servo.write(0);
    isGateOpen = false;
  }
}

String getSystemStatusJson(float temp = -999.0, float hum = -999.0) {
  bool lightState = digitalRead(BLUE_LED_PIN);
  String json = "{";
  json += "\"mode\":\"" + String(isAutoMode ? "AUTO" : "MANUAL") + "\",";
  json += "\"light_status\":\"" + String(lightState ? "ON" : "OFF") + "\",";
  json += "\"fan_status\":\"" + String(isFanOn ? "ON" : "OFF") + "\",";
  json += "\"gate_status\":\"" + String(isGateOpen ? "OPEN" : "CLOSED") + "\",";
  json += "\"temp_threshold\":" + String(TEMP_THRESHOLD);
  
  if (temp != -999.0 && hum != -999.0) {
    json += ",\"temperature\":" + String(temp);
    json += ",\"humidity\":" + String(hum);
  }
  else{
    json += ",\"temperature\": \"NULL\"";
    json += ",\"humidity\": \"Null\"";
  }
  
  json += "}";
  return json;
}

void checkTemperatureAutoControl() {
  // Only evaluate auto temperature logic if system is in AUTO mode
  if (!isAutoMode) return;

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Warning: Failed to read from DHT sensor!");
    return;
  }

  currentTemp = temp;
  currentHum = hum;

  if (currentTemp >= TEMP_THRESHOLD && !isFanOn) {
    Serial.println("[AUTO CONTROL] Temp >= Threshold. Fan ON.");
    setFanState(true);
  } else if (currentTemp < (TEMP_THRESHOLD - 1.0) && isFanOn) {
    Serial.println("[AUTO CONTROL] Temp cooled. Fan OFF.");
    setFanState(false);
  }
}

// Non-blocking Gate Auto-Close Timer
void checkGateAutoClose() {
  if (isGateOpen && (millis() - gateOpenStartTime >= GATE_OPEN_DURATION)) {
    Serial.println("[AUTO GATE] Closing gate...");
    setGateState(false);
  }
}

void handleGateOpen() {
  Serial.println("Received command: Open Gate");
  
  // Open gate and turn on fan + light
  setGateState(true);
  setLightState(true);
  setFanState(true);

  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void handleModeControl() {
  if (server.hasArg("state") || server.hasArg("mode")) {
    String modeParam = server.hasArg("state") ? server.arg("state") : server.arg("mode");
    modeParam.toLowerCase();

    if (modeParam == "auto") {
      isAutoMode = true;
      Serial.println("[MODE] System set to AUTO");
      server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
    } else if (modeParam == "manual") {
      isAutoMode = false;
      Serial.println("[MODE] System set to MANUAL");
      server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode. Use 'auto' or 'manual'.\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'state' query parameter.\"}");
  }
}

void handleTurnOnLight() {
  setLightState(true);
  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void handleTurnOffLight() {
  setLightState(false);
  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void handleFanControl() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      setFanState(true);
      server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
    } else if (state == "off") {
      setFanState(false);
      server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid state parameter.\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'state' query parameter.\"}");
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
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", getSystemStatusJson(currentTemp, currentHum));
}

void setup() {
  Serial.begin(115200);

  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(YLW_LED_PIN, OUTPUT);
  
  door_servo.attach(DOOR_SERVO_PIN);
  fan_servo.attach(FAN_SERVO_PIN);
  
  // Explicitly keep appliances OFF on startup
  setGateState(false);
  setFanState(false);
  setLightState(false);

  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSecond ESP8266 Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Web API Routes
  server.on("/gate_open", handleGateOpen);
  server.on("/mode", handleModeControl);
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

  checkGateAutoClose();

  if (millis() - lastSensorReadTime >= SENSOR_INTERVAL) {
    lastSensorReadTime = millis();
    checkTemperatureAutoControl();
  }
}