/**
 * Environmental Monitor — ESP32-C6-Touch-LCD-1.47
 * Sensors: SCD41 (CO2, T, RH), BME680 (T, RH, pressure, VOC).
 * First stage: init I²C and sensors, print values to Serial.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SCD4X.h>
#include <Adafruit_BME680.h>
#include "config.h"

Adafruit_SCD4X scd41;
Adafruit_BME680 bme680;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Environmental Monitor — init"));

  Wire.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
  Wire.setClock(I2C_FREQ_HZ);

  if (!scd41.begin()) {
    Serial.println(F("SCD41 not found. Check wiring and I2C address."));
  } else {
    Serial.println(F("SCD41 OK"));
    scd41.startPeriodicMeasurement();
  }

  if (!bme680.begin()) {
    Serial.println(F("BME680 not found. Check wiring and I2C address."));
  } else {
    Serial.println(F("BME680 OK"));
    bme680.setTemperatureOversampling(BME680_OS_8X);
    bme680.setHumidityOversampling(BME680_OS_2X);
    bme680.setPressureOversampling(BME680_OS_4X);
    bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme680.setGasHeater(320, 150);
  }

  Serial.println(F("Setup done. Reading every 2s."));
}

void loop() {
  const unsigned long periodMs = 2000;
  static unsigned long last = 0;
  if (millis() - last < periodMs) return;
  last = millis();

  Serial.println(F("---"));

  if (scd41.dataReady()) {
    if (scd41.readData()) {
      Serial.print(F("CO2: ")); Serial.print(scd41.getCO2()); Serial.println(F(" ppm"));
      Serial.print(F("SCD41 T: ")); Serial.print(scd41.getTemperature()); Serial.println(F(" °C"));
      Serial.print(F("SCD41 RH: ")); Serial.print(scd41.getRelativeHumidity()); Serial.println(F(" %"));
    }
  }

  if (bme680.performReading()) {
    Serial.print(F("BME680 T: ")); Serial.print(bme680.temperature); Serial.println(F(" °C"));
    Serial.print(F("BME680 RH: ")); Serial.print(bme680.humidity); Serial.println(F(" %"));
    Serial.print(F("Pressure: ")); Serial.print(bme680.pressure / 100.0); Serial.println(F(" hPa"));
    Serial.print(F("Gas: ")); Serial.print(bme680.gas_resistance); Serial.println(F(" Ω"));
  }

  delay(10);
}
