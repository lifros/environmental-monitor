/**
 * Environmental monitor — SCD41 (CO2, T, RH) + BME680 (T, RH, pressure, gas).
 * Console output via Serial; config in config.h.
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

// IAQ (Indoor Air Quality) index 0-500 from BME680 gas resistance (lower = better)
static void bme680IAQ(uint32_t gasOhm, int& iaq, const __FlashStringHelper*& label) {
  iaq = (gasOhm <= IAQ_R_MIN) ? 500 : (gasOhm >= IAQ_R_MAX) ? 0
      : (int)(500 - (long)(gasOhm - IAQ_R_MIN) * 500 / (long)(IAQ_R_MAX - IAQ_R_MIN));
  iaq = (iaq < 0) ? 0 : (iaq > 500) ? 500 : iaq;
  if (iaq <= 50)       label = F("Excellent");
  else if (iaq <= 100) label = F("Good");
  else if (iaq <= 150) label = F("Lightly polluted");
  else if (iaq <= 200) label = F("Moderately polluted");
  else if (iaq <= 300) label = F("Heavily polluted");
  else                 label = F("Severely polluted");
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
    // Self-heating compensation (chip recalculates T and RH)
    if (scd41.setTemperatureOffset(SCD41_TEMP_OFFSET_C) == 0) {
      Serial.print(F("SCD41: temp offset "));
      Serial.print(SCD41_TEMP_OFFSET_C);
      Serial.println(F(" °C"));
    }
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
    int iaq;
    const __FlashStringHelper* iaqLabel;
    bme680IAQ(bme.gas_resistance, iaq, iaqLabel);
    Serial.print(F("BME680 — T: "));
    Serial.print(bme.temperature, 1);
    Serial.print(F(" °C  RH: "));
    Serial.print(bme.humidity, 1);
    Serial.print(F(" %  P: "));
    Serial.print(bme.pressure / 100.0f, 1);
    Serial.print(F(" hPa  gas: "));
    Serial.print(bme.gas_resistance);
    Serial.print(F(" Ω  IAQ: "));
    Serial.print(iaq);
    Serial.print(F(" ("));
    Serial.print(iaqLabel);
    Serial.println(F(")"));
  }

  delay(60000);
}
