#include <Wire.h>

void setup() {
  Serial.begin(115200);
  
  // ESP8266 I2C pins: SDA = D2 (GPIO4), SCL = D1 (GPIO5)
  Wire.begin(4, 5); 
  
  Serial.println("\n--- Scanning Shared I2C Bus (D1 & D2) ---");
}

void loop() {
  byte error, address;
  int count = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found I2C device at address: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      count++;
    }
  }

  if (count == 0) {
    Serial.println("No I2C devices found! Check wiring and power.");
  } else {
    Serial.print("Done. Found ");
    Serial.print(count);
    Serial.println(" device(s).\n");
  }

  delay(5000);
}