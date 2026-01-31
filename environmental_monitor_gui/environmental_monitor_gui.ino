/**
 * Environmental monitor — SCD41 + BME680 with LCD (172×320).
 * Serial output + display. Config in config.h.
 */
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BME680.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"

SensirionI2cScd4x scd41;
Adafruit_BME680 bme;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

bool hasScd41 = false;
bool hasBme680 = false;

// Release I2C bus if a slave is holding SDA/SCL after master reset
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

// Use library height after rotation. Draw from bottom so content reads correctly when viewed from behind.
static void drawScreen(uint16_t co2, float tScd, float rhScd, float tBme, float rhBme, float p, int iaq, const __FlashStringHelper* iaqLabel) {
  tft.fillScreen(ST77XX_BLACK);
  const int lineH = 10;
  const int titleH = 16;
  const int h = tft.height();
  int n = (hasScd41 ? 2 : 0) + (hasBme680 ? 4 : 0);
  int y = h - titleH - n * lineH;
  if (y < 0) y = 0;

  tft.setTextSize(1);
  if (hasBme680) {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.setCursor(0, y);
    tft.print(F("IAQ: "));
    tft.print(iaq);
    tft.print(F(" ("));
    tft.print(iaqLabel);
    tft.println(F(")"));
    y += lineH;
    tft.setCursor(0, y);
    tft.print(F("P: "));
    tft.print(p, 0);
    tft.println(F(" hPa"));
    y += lineH;
    tft.setCursor(0, y);
    tft.print(F("BME T: "));
    tft.print(tBme, 1);
    tft.print(F(" C  RH: "));
    tft.print(rhBme, 1);
    tft.println(F(" %"));
    y += lineH;
  }
  if (hasScd41) {
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(0, y);
    tft.print(F("T: "));
    tft.print(tScd, 1);
    tft.print(F(" C  RH: "));
    tft.print(rhScd, 1);
    tft.println(F(" %"));
    y += lineH;
    tft.setCursor(0, y);
    tft.print(F("CO2: "));
    tft.print(co2);
    tft.println(F(" ppm"));
    y += lineH;
  }
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, y);
  tft.println(F("Env Monitor"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Environmental monitor — SCD41 + BME680 + LCD"));
  Serial.println(F("Waiting for sensors (2 s)..."));
  delay(2000);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init(TFT_W, TFT_H);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, tft.height() / 2 - 8);
  tft.println(F("Starting..."));

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
      Serial.println(F("SCD41: not found. Skipping."));
      break;
    }
    Serial.println(F("SCD41: retry..."));
    delay(400);
  }
  if (retry < maxRetries) {
    Serial.print(F("SCD41: found 0x"));
    Serial.println(serialNumber, HEX);
    delay(100);
    if (scd41.startPeriodicMeasurement() == 0) {
      hasScd41 = true;
      Serial.println(F("SCD41: measuring."));
    }
  }

  if (!bme.begin(0x77)) {
    Serial.println(F("BME680: not found."));
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
  uint16_t co2 = 0;
  float tScd = 0, rhScd = 0;
  float tBme = 0, rhBme = 0, p = 0;
  int iaq = 0;
  const __FlashStringHelper* iaqLabel = F("—");

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
    if (scd41.readMeasurement(co2, tScd, rhScd) != 0) {
      Serial.println(F("SCD41: read error"));
      delay(5000);
      return;
    }
    Serial.print(F("SCD41 — CO2: "));
    Serial.print(co2);
    Serial.print(F(" ppm  T: "));
    Serial.print(tScd, 1);
    Serial.print(F(" °C  RH: "));
    Serial.print(rhScd, 1);
    Serial.println(F(" %"));
  }

  if (hasBme680 && bme.performReading()) {
    tBme = bme.temperature;
    rhBme = bme.humidity;
    p = bme.pressure / 100.0f;
    bme680IAQ(bme.gas_resistance, iaq, iaqLabel);
    Serial.print(F("BME680 — T: "));
    Serial.print(tBme, 1);
    Serial.print(F(" °C  RH: "));
    Serial.print(rhBme, 1);
    Serial.print(F(" %  P: "));
    Serial.print(p, 1);
    Serial.print(F(" hPa  IAQ: "));
    Serial.print(iaq);
    Serial.print(F(" "));
    Serial.println(iaqLabel);
  }

  if (hasScd41 || hasBme680)
    drawScreen(co2, tScd, rhScd, tBme, rhBme, p, iaq, iaqLabel);

  delay(60000);
}
