/**
 * Environmental Monitor — ESP32-C6-Touch-LCD-1.47
 * Sensors: SCD41 (CO2, T, RH), BME680 (T, RH, pressure, VOC).
 * Upload with Arduino IDE. Reads sensors and prints to Serial.
 * SCD41: official Sensirion I2C SCD4x library (Library Manager: "Sensirion I2C SCD4x").
 */

#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BME680.h>
#include "config.h"
#include "air_quality.h"

#define SCD41_I2C_ADDR 0x62

SensirionI2cScd4x scd41;
Adafruit_BME680 bme680;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Environmental Monitor — init"));

  Wire.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
  Wire.setClock(I2C_FREQ_HZ);

  scd41.begin(Wire, SCD41_I2C_ADDR);
  int16_t err = scd41.startPeriodicMeasurement();
  if (err != 0) {
    Serial.println(F("SCD41 not found or error. Check wiring and I2C address."));
  } else {
    Serial.println(F("SCD41 OK"));
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

  float co2_ppm = 0;
  float temp_c = 0;
  float rh_percent = 0;
  bool has_co2 = false;

  bool dataReady = false;
  if (scd41.getDataReadyStatus(dataReady) == 0 && dataReady) {
    uint16_t co2;
    float temperature, humidity;
    if (scd41.readMeasurement(co2, temperature, humidity) == 0) {
      co2_ppm = (float)co2;
      temp_c = temperature;
      rh_percent = humidity;
      has_co2 = true;
      Serial.print(F("CO2: ")); Serial.print(co2); Serial.println(F(" ppm"));
      Serial.print(F("SCD41 T: ")); Serial.print(temperature); Serial.println(F(" °C"));
      Serial.print(F("SCD41 RH: ")); Serial.print(humidity); Serial.println(F(" %"));
    }
  }

  bool has_bme = false;
  float pressure_hpa = 0;
  uint32_t gas_ohm = 0;
  if (bme680.performReading()) {
    has_bme = true;
    if (!has_co2) {
      temp_c = bme680.temperature;
      rh_percent = bme680.humidity;
    }
    pressure_hpa = bme680.pressure / 100.0f;
    gas_ohm = (uint32_t)bme680.gas_resistance;
    Serial.print(F("BME680 T: ")); Serial.print(bme680.temperature); Serial.println(F(" °C"));
    Serial.print(F("BME680 RH: ")); Serial.print(bme680.humidity); Serial.println(F(" %"));
    Serial.print(F("Pressure: ")); Serial.print(pressure_hpa); Serial.println(F(" hPa"));
    Serial.print(F("Gas: ")); Serial.print(gas_ohm); Serial.println(F(" Ω"));
  }

  // Air quality estimate
  AirQualityInput aqIn = {};
  aqIn.has_co2 = has_co2;
  aqIn.co2_ppm = co2_ppm;
  aqIn.temperature_c = temp_c;
  aqIn.humidity_percent = rh_percent;
  aqIn.pressure_hpa = pressure_hpa;
  aqIn.gas_resistance_ohm = gas_ohm;
  AirQualityEstimate aq = estimateAirQuality(aqIn);
  Serial.print(F("Air quality: ")); Serial.print(aq.score); Serial.print(F("/100 — "));
  Serial.println(aq.category);
  Serial.println(aq.suggestion);

  delay(10);
}
