/**
 * I2C scanner for ESP32-C6-Touch-LCD-1.47 — onboard devices only.
 * SDA=18, SCL=19 (expansion header). LCD is SPI, not I2C.
 */
#include <Wire.h>

#define I2C_SDA_GPIO 18
#define I2C_SCL_GPIO 19

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("I2C Scanner — ESP32-C6-Touch-LCD-1.47 (onboard only)"));
  Serial.println(F("Onboard: 0x63 touch, 0x6B IMU/touch"));
  Serial.println();


  Wire.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
  delay(200);
}

void loop() {
  Serial.println(F("Scanning 0x08..0x77..."));
  int count = 0;

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
      count++;
      Serial.print(F("  [0x"));
      if (addr < 0x10) Serial.print('0');
      Serial.print(addr, HEX);
      Serial.print(F("] "));
      Serial.println(label(addr));
    }
    delay(5);
  }

  Serial.print(count);
  Serial.println(F(" device(s) found."));
  Serial.println();
  delay(5000);
}

const __FlashStringHelper* label(uint8_t addr) {
  switch (addr) {
    case 0x63: return F("onboard touch");
    case 0x6B: return F("onboard IMU / touch");
    default:   return F("—");
  }
}
