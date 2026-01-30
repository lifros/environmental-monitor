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

  Serial.println(F("Setup done. Report only when new CO2 is ready (~5s)."));
}

void loop() {
  const unsigned long periodMs = 2000;
  static unsigned long last = 0;
  if (millis() - last < periodMs) return;
  last = millis();

  static float last_co2_ppm = 0;
  static float last_temp_c = 0;
  static float last_rh_percent = 0;
  static bool scd41_ever_read = false;

  float co2_ppm = last_co2_ppm;
  float temp_c = last_temp_c;
  float rh_percent = last_rh_percent;
  bool has_co2 = scd41_ever_read;
  bool new_co2 = false;

  bool dataReady = false;
  if (scd41.getDataReadyStatus(dataReady) == 0 && dataReady) {
    uint16_t co2;
    float temperature, humidity;
    if (scd41.readMeasurement(co2, temperature, humidity) == 0) {
      last_co2_ppm = co2_ppm = (float)co2;
      last_temp_c = temp_c = temperature;
      last_rh_percent = rh_percent = humidity;
      scd41_ever_read = has_co2 = true;
      new_co2 = true;
    }
  }

  if (!bme680.performReading())
    return;

  float pressure_hpa = bme680.pressure / 100.0f;
  uint32_t gas_ohm = (uint32_t)bme680.gas_resistance;
  if (!scd41_ever_read) {
    temp_c = bme680.temperature;
    rh_percent = bme680.humidity;
  }

  // Single report only when new CO2 is available (~every 5s)
  if (!new_co2)
    return;

  AirQualityInput aqIn = {};
  aqIn.has_co2 = has_co2;
  aqIn.co2_ppm = co2_ppm;
  aqIn.temperature_c = temp_c;
  aqIn.humidity_percent = rh_percent;
  aqIn.pressure_hpa = pressure_hpa;
  aqIn.gas_resistance_ohm = gas_ohm;
  AirQualityEstimate aq = estimateAirQuality(aqIn);

  float t_bme = bme680.temperature;
  float rh_bme = bme680.humidity;

  Serial.println(F("---"));
  Serial.print(F("CO2: ")); Serial.print((uint16_t)co2_ppm); Serial.println(F(" ppm"));
  Serial.print(F("T: ")); Serial.print(temp_c, 1); Serial.print(F(" °C (SCD41)  |  "));
  Serial.print(t_bme, 1); Serial.println(F(" °C (BME680)"));
  Serial.print(F("RH: ")); Serial.print(rh_percent, 1); Serial.print(F(" % (SCD41)  |  "));
  Serial.print(rh_bme, 1); Serial.println(F(" % (BME680)"));
  Serial.print(F("Pressure: ")); Serial.print(pressure_hpa, 1); Serial.print(F(" hPa  |  Gas: "));
  Serial.print(gas_ohm); Serial.println(F(" Ω"));
  Serial.print(F("Air quality: ")); Serial.print(aq.score); Serial.print(F("/100 — "));
  Serial.print(aq.category); Serial.print(F("  |  ")); Serial.println(aq.suggestion);

  delay(10);
}
