/**
 * Environmental monitor — SCD41 (CO2, T, RH) + BME680 (T, RH, pressure, gas).
 */
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BME680.h>
#include "config.h"

SensirionI2cScd4x scd41;
Adafruit_BME680 bme;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Environmental monitor — SCD41 + BME680"));

  Wire.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
  delay(200);

  scd41.begin(Wire, 0x62);
  delay(200);

  uint16_t serial0, serial1, serial2;
  if (scd41.getSerialNumber(serial0, serial1, serial2) != 0) {
    Serial.println(F("SCD41: not found on I2C (addr 0x62). Check SDA=18, SCL=19, 3V3, GND and wiring."));
    for (;;) delay(1000);
  }
  Serial.print(F("SCD41: found (serial ..."));
  Serial.print(serial2, HEX);
  Serial.println(F(")."));

  scd41.stopPeriodicMeasurement();
  delay(500);

  if (scd41.startPeriodicMeasurement() != 0) {
    Serial.println(F("SCD41: startPeriodicMeasurement failed."));
    for (;;) delay(1000);
  }
  Serial.println(F("SCD41: measuring (first result in ~5 s)."));

  if (!bme.begin(0x77)) {
    Serial.println(F("BME680: not found at 0x77."));
  } else {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
    Serial.println(F("BME680: ok."));
  }
}

void loop() {
  bool dataReady = false;
  if (scd41.getDataReadyStatus(dataReady) != 0) {
    delay(1000);
    return;
  }
  if (!dataReady) {
    delay(1000);
    return;
  }

  uint16_t co2;
  float temperature;
  float humidity;
  if (scd41.readMeasurement(co2, temperature, humidity) != 0) {
    Serial.println(F("SCD41: read error"));
    delay(5000);
    return;
  }

  Serial.print(F("SCD41 — CO2: "));
  Serial.print(co2);
  Serial.print(F(" ppm  T: "));
  Serial.print(temperature, 1);
  Serial.print(F(" °C  RH: "));
  Serial.print(humidity, 1);
  Serial.println(F(" %"));

  if (bme.performReading()) {
    Serial.print(F("BME680 — T: "));
    Serial.print(bme.temperature, 1);
    Serial.print(F(" °C  RH: "));
    Serial.print(bme.humidity, 1);
    Serial.print(F(" %  P: "));
    Serial.print(bme.pressure / 100.0f, 1);
    Serial.print(F(" hPa  gas: "));
    Serial.print(bme.gas_resistance);
    Serial.println(F(" Ω"));
  }

  delay(5000);
}
