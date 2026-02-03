/**
 * Environmental monitor — SCD41 CO2 + T/RH with LCD (172×320).
 * Serial output + display. Optional WiFi + MQTT for Home Assistant.
 * Config in config.h. Display: Arduino_GFX_Library with official board init sequence.
 */
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

#if ENABLE_MQTT
#include <WiFi.h>
#include <PubSubClient.h>
#endif
#if ENABLE_OTA
#include <ArduinoOTA.h>
#endif

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, TFT_RST, 0 /* rotation set later */, false /* IPS */,
  TFT_W, TFT_H,
  TFT_COL_OFFSET, TFT_ROW_OFFSET, TFT_COL_OFFSET, TFT_ROW_OFFSET);

SensirionI2cScd4x scd41;

#if ENABLE_MQTT
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
static char mqttStateTopic[48];
static bool mqttDiscoveryPublished = false;
#endif

bool hasScd41 = false;

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

#if ENABLE_MQTT
static void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print(F("WiFi connecting to "));
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi OK "));
    Serial.println(WiFi.localIP());
  } else Serial.println(F("WiFi failed"));
}

static void mqttPublishDiscovery() {
  if (mqttDiscoveryPublished || !mqtt.connected()) return;
  const char* stateTopic = mqttStateTopic;
  const char* devId = MQTT_DEVICE_ID;
  char topic[80];
  char payload[320];
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_co2/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon CO2\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.co2 }}\","
    "\"unit_of_measurement\":\"ppm\",\"device_class\":\"carbon_dioxide\",\"unique_id\":\"%s_co2\"}",
    stateTopic, devId);
  if (!mqtt.publish(topic, payload, true)) return;
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_temp_scd41/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon Temp (SCD41)\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.temperature_scd41 }}\","
    "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\",\"unique_id\":\"%s_temp_scd41\"}",
    stateTopic, devId);
  if (!mqtt.publish(topic, payload, true)) return;
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_humidity_scd41/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon Humidity (SCD41)\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.humidity_scd41 }}\","
    "\"unit_of_measurement\":\"%%\",\"device_class\":\"humidity\",\"unique_id\":\"%s_humidity_scd41\"}",
    stateTopic, devId);
  if (!mqtt.publish(topic, payload, true)) return;
  mqttDiscoveryPublished = true;
  Serial.println(F("MQTT discovery published"));
}

static void mqttConnect() {
  if (mqtt.connected()) return;
  wifiConnect();
  if (WiFi.status() != WL_CONNECTED) return;
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setBufferSize(512);
  Serial.print(F("MQTT connecting to "));
  Serial.println(MQTT_BROKER);
  bool ok = (strlen(MQTT_USER) > 0)
    ? mqtt.connect(MQTT_DEVICE_ID, MQTT_USER, MQTT_PASSWORD)
    : mqtt.connect(MQTT_DEVICE_ID);
  if (ok) {
    Serial.println(F("MQTT connected"));
    mqttPublishDiscovery();
  } else Serial.println(mqtt.state());
}

static void mqttPublishState(uint16_t co2, float tScd, float rhScd) {
  if (!mqtt.connected()) return;
  char payload[120];
  int n = snprintf(payload, sizeof(payload),
    "{\"co2\":%u,\"temperature_scd41\":%.1f,\"humidity_scd41\":%.1f}",
    co2, tScd, rhScd);
  if (n > 0 && (size_t)n < sizeof(payload))
    mqtt.publish(mqttStateTopic, payload, false);
}
#endif

// CO2 quality bands (indoor air): label and color for status line
static void getCo2Quality(uint16_t ppm, const char*& label, uint16_t& color) {
  if (ppm <= 800)       { label = "Good";      color = (uint16_t)0x07E0; }
  else if (ppm <= 1000) { label = "Moderate";  color = (uint16_t)0xFFE0; }
  else if (ppm <= 2000) { label = "Poor";      color = (uint16_t)0xFD20; }
  else                  { label = "Unhealthy"; color = (uint16_t)0xF800; }
}

// Redraw only the countdown bar at bottom (progress + seconds)
static void drawCountdown(int secondsLeft, int totalSec) {
  const int h = 24;
  const int y0 = gfx->height() - h;
  const int w = gfx->width();
  gfx->fillRect(0, y0, w, h, RGB565_BLACK);
  // Progress bar: filled portion = (totalSec - secondsLeft) / totalSec
  if (totalSec > 0) {
    const int barW = w - 4;
    const int filled = (int)((long)(totalSec - secondsLeft) * barW / totalSec);
    if (filled > 0)
      gfx->fillRect(2, y0 + 8, filled, 10, (uint16_t)0x3186);
    gfx->drawRect(2, y0 + 8, barW, 10, (uint16_t)0x4A49);
  }
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_YELLOW, RGB565_BLACK);
  gfx->setCursor(4, y0 + 2);
  gfx->print(secondsLeft);
  gfx->print(F(" s"));
}

#if ENABLE_MQTT
// WiFi/MQTT status: top-right. Green = both, yellow = WiFi only, gray = off.
static void drawConnectionIndicator(bool wifiOk, bool mqttOk) {
  const int boxW = 22;
  const int boxH = 22;
  const int x0 = gfx->width() - boxW - 4;
  const int y0 = 4;
  const int cx = x0 + boxW / 2;
  const int cy = y0 + boxH / 2;
  const int r = 7;
  gfx->fillRect(x0, y0, boxW, boxH, RGB565_BLACK);
  uint16_t color = mqttOk ? (uint16_t)0x07E0 : (wifiOk ? (uint16_t)0xFFE0 : (uint16_t)0x3186);
  gfx->fillCircle(cx, cy, r, color);
}
#endif

// Optimized layout: header, large CO2 + quality, T/RH row, countdown bar
static void drawScreen(uint16_t co2, float tScd, float rhScd, int nextInSec) {
  gfx->fillScreen(RGB565_BLACK);
  const int mx = 8;
  const int w = gfx->width();

  // Header: title left, connection right (text size 3)
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(mx, 4);
  gfx->print(F("CO2 Monitor"));
#if ENABLE_MQTT
  drawConnectionIndicator(WiFi.status() == WL_CONNECTED, mqtt.connected());
#endif

  if (!hasScd41) {
    drawCountdown(nextInSec, MEASURE_INTERVAL_SEC);
    return;
  }

  // Main: very large CO2 value (text size 5)
  gfx->setTextSize(5);
  gfx->setTextColor(RGB565_CYAN, RGB565_BLACK);
  int xCo2 = mx;
  int yCo2 = 38;
  gfx->setCursor(xCo2, yCo2);
  gfx->print(co2);
  gfx->setTextSize(3);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->print(F(" ppm"));

  // Quality label with color dot (text size 3); clear full row so "Unhealthy" fits
  const char* qLabel;
  uint16_t qColor;
  getCo2Quality(co2, qLabel, qColor);
  const int yQuality = yCo2 + 52;
  const int lineH = 24;
  gfx->fillRect(0, yQuality, w, lineH, RGB565_BLACK);
  gfx->fillRect(xCo2, yQuality + 4, 10, 10, qColor);
  gfx->setCursor(xCo2 + 14, yQuality + 2);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->print(qLabel);

  // T and RH: one row, text size 3; fixed columns so they don't overlap
  const int yTrh = yQuality + lineH + 4;
  gfx->fillRect(0, yTrh, w, lineH, RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setTextColor((uint16_t)0xAD55, RGB565_BLACK);  /* temp tint */
  gfx->setCursor(mx, yTrh + 2);
  gfx->print(F("T "));
  gfx->print(tScd, 1);
  gfx->print(F(" C"));
  gfx->setTextColor((uint16_t)0x07FF, RGB565_BLACK);  /* cyan for RH */
  const int rhX = (w / 2) + 2;
  gfx->fillCircle(rhX + 6, yTrh + 4 + 6, 6, (uint16_t)0x07FF);  /* small circle = humidity indicator */
  gfx->setCursor(rhX + 16, yTrh + 2);
  gfx->print(F("RH "));
  gfx->print(rhScd, 1);
  gfx->print(F(" %"));

  drawCountdown(nextInSec, MEASURE_INTERVAL_SEC);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("CO2 monitor — SCD41 + LCD"));
  Serial.println(F("Waiting for sensor (2 s)..."));
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

#if ENABLE_MQTT
  snprintf(mqttStateTopic, sizeof(mqttStateTopic), "%s/state", MQTT_DEVICE_ID);
#endif

#if ENABLE_OTA && ENABLE_MQTT
  wifiConnect();
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    if (strlen(OTA_PASSWORD) > 0)
      ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]() {
      Serial.println(F("OTA: Start"));
    });
    ArduinoOTA.onEnd([]() {
      Serial.println(F("\nOTA: End"));
    });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
      Serial.printf("OTA: %u%%\r", (done * 100) / total);
    });
    ArduinoOTA.onError([](ota_error_t err) {
      Serial.printf("OTA Error %u\n", (unsigned)err);
    });
    ArduinoOTA.begin();
    Serial.print(F("OTA: "));
    Serial.println(WiFi.localIP());
  }
#endif

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
#if SCD41_SENSOR_ALTITUDE_M > 0
    if (scd41.setSensorAltitude(SCD41_SENSOR_ALTITUDE_M) == 0) {
      Serial.print(F("SCD41: altitude "));
      Serial.print(SCD41_SENSOR_ALTITUDE_M);
      Serial.println(F(" m"));
    }
#endif
#if SCD41_PERSIST_SETTINGS
    if (scd41.persistSettings() == 0) {
      Serial.println(F("SCD41: settings persisted"));
      delay(800);  // persist_settings duration per datasheet
    }
#endif
    delay(500);  // let sensor accept config before starting measurement
#if SCD41_LOW_POWER_PERIODIC
    if (scd41.startLowPowerPeriodicMeasurement() == 0) {
      hasScd41 = true;
      Serial.println(F("SCD41: measuring (low power, ~30 s)."));
    }
#else
    if (scd41.startPeriodicMeasurement() == 0) {
      hasScd41 = true;
      Serial.println(F("SCD41: measuring."));
    }
#endif
  }
}

// SCD41 valid range per Sensirion datasheet Table 2 & 3: T -10..60 °C, RH 0..100%;
// invalid/not-ready often reported as T = -45 °C, RH = 100 % (raw 0 / 0xFFFF)
static bool scd41Valid(float tC, float rh) {
  if (tC <= -44.0f && tC >= -46.0f && rh >= 99.5f) return false;  // sentinel
  return (tC >= -10.0f && tC <= 60.0f && rh >= 0.0f && rh <= 100.0f);
}

void loop() {
  static uint16_t lastCo2 = 0;
  static float lastTScd = 0, lastRhScd = 0;
  static bool haveValidScd41 = false;

  uint16_t co2 = lastCo2;
  float tScd = lastTScd, rhScd = lastRhScd;

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
    // read_measurement: CO2 [ppm], T [°C], RH [%] per datasheet Table 11
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

  if (hasScd41) {
#if ENABLE_MQTT
    mqttConnect();
    mqttPublishState(co2, tScd, rhScd);
#endif
#if ENABLE_OTA
    ArduinoOTA.handle();
#endif
    drawScreen(co2, tScd, rhScd, MEASURE_INTERVAL_SEC);
    for (int s = MEASURE_INTERVAL_SEC - 1; s >= 0; s--) {
#if ENABLE_MQTT
      mqtt.loop();
      drawConnectionIndicator(WiFi.status() == WL_CONNECTED, mqtt.connected());
#endif
#if ENABLE_OTA
      ArduinoOTA.handle();
#endif
      drawCountdown(s, MEASURE_INTERVAL_SEC);
      delay(1000);
    }
  } else {
#if ENABLE_OTA
    ArduinoOTA.handle();
#endif
    delay(5000);
  }
}
