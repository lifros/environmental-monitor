/**
 * Environmental monitor — SCD41 (CO2, T, RH) + BME680 (T, RH, pressure, gas).
 */
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BME680.h>
#include "config.h"

SensirionI2cScd4x scd41;
Adafruit_BME680 bme;

bool hasScd41 = false;
bool hasBme680 = false;

// Release I2C bus if a slave is holding SDA/SCL after master reset (e.g. ESP32 reset)
static void i2cBusRecovery() {
  pinMode(I2C_SDA_GPIO, OUTPUT);
  pinMode(I2C_SCL_GPIO, OUTPUT);
  digitalWrite(I2C_SDA_GPIO, HIGH);
  digitalWrite(I2C_SCL_GPIO, HIGH);
  delay(10);
  for (int i = 0; i < 9; i++) {
    digitalWrite(I2C_SCL_GPIO, LOW);
    delayMicroseconds(5);
    digitalWrite(I2C_SCL_GPIO, HIGH);
    delayMicroseconds(5);
  }
  pinMode(I2C_SDA_GPIO, INPUT);
  pinMode(I2C_SCL_GPIO, INPUT);
  delay(10);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Environmental monitor — SCD41 + BME680"));
  Serial.println(F("Waiting for sensors (2 s)..."));
  delay(2000);

  i2cBusRecovery();
  Wire.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
  delay(200);

  scd41.begin(Wire, 0x62);
  delay(200);

  scd41.stopPeriodicMeasurement();
  delay(500);

  uint64_t serialNumber;
  const int maxRetries = 8;
  int retry = 0;
  while (scd41.getSerialNumber(serialNumber) != 0) {
    retry++;
    if (retry >= maxRetries) {
      Serial.println(F("SCD41: not found on I2C (addr 0x62). Skipping."));
      break;
    }
    Serial.println(F("SCD41: retry..."));
    delay(400);
  }
  if (retry < maxRetries) {
    Serial.print(F("SCD41: found (serial 0x"));
    Serial.print(serialNumber, HEX);
    Serial.println(F(")."));
    delay(100);
    if (scd41.startPeriodicMeasurement() == 0) {
      hasScd41 = true;
      Serial.println(F("SCD41: measuring (first result in ~5 s, then every 60 s)."));
    } else {
      Serial.println(F("SCD41: startPeriodicMeasurement failed."));
    }
  }

  if (!bme.begin(0x77)) {
    Serial.println(F("BME680: not found at 0x77."));
  } else {
    hasBme680 = true;
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
    Serial.println(F("BME680: ok."));
  }
}

void loop() {
  if (hasScd41) {
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
  }

  if (hasBme680 && bme.performReading()) {
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

  delay(60000);
}
