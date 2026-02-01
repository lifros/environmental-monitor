/**
 * Environmental monitor — SCD41 + BME680 with LCD (172×320).
 * Serial output + display. Config in config.h.
 * Display: Arduino_GFX_Library with official board init sequence.
 */
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BME680.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, TFT_RST, 0 /* rotation set later */, false /* IPS */,
  TFT_W, TFT_H,
  TFT_COL_OFFSET, TFT_ROW_OFFSET, TFT_COL_OFFSET, TFT_ROW_OFFSET);

SensirionI2cScd4x scd41;
Adafruit_BME680 bme;

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

// Official init sequence for ESP32-C6-Touch-LCD-1.47 (172×320)
static void lcd_reg_init(void) {
  static const uint8_t init_operations[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    DELAY, 120,

    BEGIN_WRITE,
    WRITE_C8_D16, 0xDF, 0x98, 0x53,
    WRITE_C8_D8, 0xB2, 0x23,

    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 4,
    0x00, 0x47, 0x00, 0x6F,

    WRITE_COMMAND_8, 0xBB,
    WRITE_BYTES, 6,
    0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,

    WRITE_C8_D16, 0xC0, 0x44, 0xA4,
    WRITE_C8_D8, 0xC1, 0x16,

    WRITE_COMMAND_8, 0xC3,
    WRITE_BYTES, 8,
    0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,

    WRITE_COMMAND_8, 0xC4,
    WRITE_BYTES, 12,
    0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,

    WRITE_COMMAND_8, 0xC8,
    WRITE_BYTES, 32,
    0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,

    WRITE_COMMAND_8, 0xD0,
    WRITE_BYTES, 5,
    0x04, 0x06, 0x6B, 0x0F, 0x00,

    WRITE_C8_D16, 0xD7, 0x00, 0x30,
    WRITE_C8_D8, 0xE6, 0x14,
    WRITE_C8_D8, 0xDE, 0x01,

    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 5,
    0x03, 0x13, 0xEF, 0x35, 0x35,

    WRITE_COMMAND_8, 0xC1,
    WRITE_BYTES, 3,
    0x14, 0x15, 0xC0,

    WRITE_C8_D16, 0xC2, 0x06, 0x3A,
    WRITE_C8_D16, 0xC4, 0x72, 0x12,
    WRITE_C8_D8, 0xBE, 0x00,
    WRITE_C8_D8, 0xDE, 0x02,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x00, 0x02, 0x00,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x01, 0x02, 0x00,

    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x35, 0x00,
    WRITE_C8_D8, 0x3A, 0x05,

    WRITE_COMMAND_8, 0x2A,
    WRITE_BYTES, 4,
    0x00, 0x22, 0x00, 0xCD,

    WRITE_COMMAND_8, 0x2B,
    WRITE_BYTES, 4,
    0x00, 0x00, 0x01, 0x3F,

    WRITE_C8_D8, 0xDE, 0x02,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x00, 0x02, 0x00,

    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_COMMAND_8, 0x21,
    END_WRITE,

    DELAY, 10,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29,
    END_WRITE
  };
  bus->batchOperation(init_operations, sizeof(init_operations));
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

// Redraw only the countdown line (uses current rotation: bottom of visible area)
static void drawCountdown(int secondsLeft) {
  const int h = 24;
  const int countdownY = gfx->height() - h;
  gfx->fillRect(0, countdownY, gfx->width(), h, RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_YELLOW, RGB565_BLACK);
  gfx->setCursor(10, countdownY);
  gfx->print(F("Next in: "));
  gfx->print(secondsLeft);
  gfx->println(F(" s"));
}

// Normal layout: title at top, then SCD41, then BME680 data, countdown at bottom
static void drawScreen(uint16_t co2, float tScd, float rhScd, float tBme, float rhBme, float p, int iaq, const __FlashStringHelper* iaqLabel, int nextInSec) {
  gfx->fillScreen(RGB565_BLACK);
  const int marginX = 10;
  const int marginY = 10;
  const int lineH = 16;
  int y = marginY;

  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(marginX, y);
  gfx->println(F("Env Monitor"));
  y += 24;

  gfx->setTextSize(2);
  if (hasScd41) {
    gfx->setTextColor(RGB565_CYAN, RGB565_BLACK);
    gfx->setCursor(marginX, y);
    gfx->print(F("CO2: "));
    gfx->print(co2);
    gfx->println(F(" ppm"));
    y += lineH;
    gfx->setCursor(marginX, y);
    gfx->print(F("T: "));
    gfx->print(tScd, 1);
    gfx->print(F(" C  RH: "));
    gfx->print(rhScd, 1);
    gfx->println(F(" %"));
    y += lineH;
  }
  if (hasBme680) {
    gfx->setTextColor(RGB565_GREEN, RGB565_BLACK);
    gfx->setCursor(marginX, y);
    gfx->print(F("BME T: "));
    gfx->print(tBme, 1);
    gfx->print(F(" C  RH: "));
    gfx->print(rhBme, 1);
    gfx->println(F(" %"));
    y += lineH;
    gfx->setCursor(marginX, y);
    gfx->print(F("P: "));
    gfx->print(p, 0);
    gfx->println(F(" hPa"));
    y += lineH;
    gfx->setCursor(marginX, y);
    gfx->print(F("IAQ: "));
    gfx->print(iaq);
    gfx->print(F(" ("));
    gfx->print(iaqLabel);
    gfx->println(F(")"));
  }

  drawCountdown(nextInSec);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Environmental monitor — SCD41 + BME680 + LCD"));
  Serial.println(F("Waiting for sensors (2 s)..."));
  delay(2000);

  if (!gfx->begin()) {
    Serial.println(F("gfx->begin() failed!"));
  }
  lcd_reg_init();
  gfx->setRotation(TFT_ROTATION);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(10, 10);
  gfx->println(F("Starting..."));

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
    // Self-heating compensation (chip recalculates T and RH)
    if (scd41.setTemperatureOffset(SCD41_TEMP_OFFSET_C) == 0) {
      Serial.print(F("SCD41: temp offset "));
      Serial.print(SCD41_TEMP_OFFSET_C);
      Serial.println(F(" °C"));
    }
    delay(500);  // let sensor store offset before starting measurement
    if (scd41.startPeriodicMeasurement() == 0) {
      hasScd41 = true;
      Serial.println(F("SCD41: measuring."));
    }
  }

  if (!bme.begin(0x77)) {
    Serial.println(F("BME680: not found."));
  } else {
    hasBme680 = true;
    // Oversampling and heater per Bosch/Adafruit examples; IIR filters pressure only
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);  // 320°C, 150 ms — standard for indoor IAQ
    Serial.println(F("BME680: ok. (IAQ may need 10–30 min burn-in to stabilize)"));
  }
}

// SCD41 valid range (datasheet); -45°C / 100% = invalid sentinel values
static bool scd41Valid(float tC, float rh) {
  return (tC >= -40.0f && tC <= 85.0f && rh >= 0.0f && rh < 99.5f);
}

void loop() {
  static uint16_t lastCo2 = 0;
  static float lastTScd = 0, lastRhScd = 0;
  static bool haveValidScd41 = false;

  uint16_t co2 = lastCo2;
  float tScd = lastTScd, rhScd = lastRhScd;
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
    if (!scd41Valid(tScd, rhScd)) {
      Serial.println(F("SCD41: invalid T/RH (using last valid)"));
      co2 = lastCo2;
      tScd = lastTScd;
      rhScd = lastRhScd;
    } else {
      lastCo2 = co2;
      lastTScd = tScd;
      lastRhScd = rhScd;
      haveValidScd41 = true;
      Serial.print(F("SCD41 — CO2: "));
      Serial.print(co2);
      Serial.print(F(" ppm  T: "));
      Serial.print(tScd, 1);
      Serial.print(F(" °C  RH: "));
      Serial.print(rhScd, 1);
      Serial.println(F(" %"));
    }
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

  if (hasScd41 || hasBme680) {
    drawScreen(co2, tScd, rhScd, tBme, rhBme, p, iaq, iaqLabel, MEASURE_INTERVAL_SEC);
    for (int s = MEASURE_INTERVAL_SEC - 1; s >= 0; s--) {
      drawCountdown(s);
      delay(1000);
    }
  } else {
    delay(5000);
  }
}
