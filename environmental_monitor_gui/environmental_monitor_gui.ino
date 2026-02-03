/**
 * Environmental monitor — SCD41 + BME680 with LCD (172×320).
 * Serial output + display. Optional WiFi + MQTT for Home Assistant.
 * Config in config.h. Display: Arduino_GFX_Library with official board init sequence.
 */
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <bsec2.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include "config.h"

#if ENABLE_MQTT
#include <WiFi.h>
#include <PubSubClient.h>
#endif

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, TFT_RST, 0 /* rotation set later */, false /* IPS */,
  TFT_W, TFT_H,
  TFT_COL_OFFSET, TFT_ROW_OFFSET, TFT_COL_OFFSET, TFT_ROW_OFFSET);

SensirionI2cScd4x scd41;
Bsec2 bme680;

#if ENABLE_MQTT
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
static char mqttStateTopic[48];
static bool mqttDiscoveryPublished = false;
#endif

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

// BSEC IAQ classification (official Bosch bands)
static const __FlashStringHelper* bsecIAQLabel(float iaq) {
  if (iaq <= 50)       return F("Excellent");
  if (iaq <= 100)      return F("Good");
  if (iaq <= 150)      return F("Lightly polluted");
  if (iaq <= 200)      return F("Moderately polluted");
  if (iaq <= 300)      return F("Heavily polluted");
  return F("Severely polluted");
}

static bool bmeValuesValid(float t, float rh, float p, float iaq) {
  return (t >= -40.0f && t <= 85.0f && rh >= 0.0f && rh <= 100.0f &&
          p >= 300.0f && p <= 1100.0f && iaq >= 0.0f && iaq <= 500.0f);
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
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_temp_bme/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon Temp (BME680)\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.temperature_bme }}\","
    "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\",\"unique_id\":\"%s_temp_bme\"}",
    stateTopic, devId);
  if (!mqtt.publish(topic, payload, true)) return;
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_humidity_bme/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon Humidity (BME680)\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.humidity_bme }}\","
    "\"unit_of_measurement\":\"%%\",\"device_class\":\"humidity\",\"unique_id\":\"%s_humidity_bme\"}",
    stateTopic, devId);
  if (!mqtt.publish(topic, payload, true)) return;
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_pressure/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon Pressure\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.pressure }}\","
    "\"unit_of_measurement\":\"hPa\",\"device_class\":\"pressure\",\"unique_id\":\"%s_pressure\"}",
    stateTopic, devId);
  if (!mqtt.publish(topic, payload, true)) return;
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_iaq/config", devId);
  snprintf(payload, sizeof(payload),
    "{\"name\":\"EnvMon IAQ\",\"state_topic\":\"%s\",\"value_template\":\"{{ value_json.iaq }}\","
    "\"unique_id\":\"%s_iaq\"}",
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

static void mqttPublishState(uint16_t co2, float tScd, float rhScd, float tBme, float rhBme, float p, int iaq) {
  if (!mqtt.connected()) return;
  char payload[220];
  int n = snprintf(payload, sizeof(payload),
    "{\"co2\":%u,\"temperature_scd41\":%.1f,\"humidity_scd41\":%.1f,"
    "\"temperature_bme\":%.1f,\"humidity_bme\":%.1f,\"pressure\":%.1f,\"iaq\":%d}",
    co2, tScd, rhScd, tBme, rhBme, p, iaq);
  if (n > 0 && (size_t)n < sizeof(payload))
    mqtt.publish(mqttStateTopic, payload, false);
}
#endif

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

#if ENABLE_MQTT
// WiFi/MQTT status icon: top-right corner. Green = both connected, yellow = WiFi only, gray = disconnected.
static void drawConnectionIndicator(bool wifiOk, bool mqttOk) {
  const int boxW = 24;
  const int boxH = 24;
  const int x0 = gfx->width() - boxW - 6;
  const int y0 = 6;
  const int cx = x0 + boxW / 2;
  const int cy = y0 + boxH / 2;
  const int r = 8;
  gfx->fillRect(x0, y0, boxW, boxH, RGB565_BLACK);
  uint16_t color = mqttOk ? RGB565_GREEN : (wifiOk ? RGB565_YELLOW : (uint16_t)0x3186);  /* dark gray */
  gfx->fillCircle(cx, cy, r, color);
}
#endif

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

#if ENABLE_MQTT
  drawConnectionIndicator(WiFi.status() == WL_CONNECTED, mqtt.connected());
#endif

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

#if ENABLE_MQTT
  snprintf(mqttStateTopic, sizeof(mqttStateTopic), "%s/state", MQTT_DEVICE_ID);
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
    delay(500);  // let sensor store offset before starting measurement
    if (scd41.startPeriodicMeasurement() == 0) {
      hasScd41 = true;
      Serial.println(F("SCD41: measuring."));
    }
  }

  Wire.beginTransmission(0x77);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("BME680: not found."));
  } else {
    bme680.begin(0x77, Wire);
    bsecSensor requested[] = {
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
      BSEC_OUTPUT_RAW_PRESSURE
    };
    bme680.updateSubscription(requested, sizeof(requested) / sizeof(requested[0]), BSEC_SAMPLE_RATE_LP);
    Preferences prefs;
    if (prefs.begin("bsec", true)) {
      size_t len = prefs.getBytesLength("state");
      if (len >= BSEC_MAX_STATE_BLOB_SIZE) {
        uint8_t stateBuf[BSEC_MAX_STATE_BLOB_SIZE];
        if (prefs.getBytes("state", stateBuf, BSEC_MAX_STATE_BLOB_SIZE) == BSEC_MAX_STATE_BLOB_SIZE && bme680.setState(stateBuf)) {
          Serial.println(F("BME680+BSEC: state restored."));
        }
      }
      prefs.end();
    }
    hasBme680 = true;
    Serial.println(F("BME680+BSEC: initialized. (IAQ may need 10–30 min burn-in)"));
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
  static float lastTBme = 0, lastRhBme = 0, lastP = 0;
  static int lastIaq = 0;
  static const __FlashStringHelper* lastIaqLabel = F("—");
  static bool haveValidBme = false;
  static uint8_t cycleCount = 0;

  uint16_t co2 = lastCo2;
  float tScd = lastTScd, rhScd = lastRhScd;
  float tBme = lastTBme, rhBme = lastRhBme, p = lastP;
  int iaq = lastIaq;
  const __FlashStringHelper* iaqLabel = lastIaqLabel;

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

  if (hasBme680) {
    if (bme680.run()) {
      float t = bme680.getData(BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE).signal;
      float rh = bme680.getData(BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY).signal;
      float pPa = bme680.getData(BSEC_OUTPUT_RAW_PRESSURE).signal;
      float iaqF = bme680.getData(BSEC_OUTPUT_IAQ).signal;
      if (bmeValuesValid(t, rh, pPa / 100.0f, iaqF)) {
        tBme = lastTBme = t;
        rhBme = lastRhBme = rh;
        p = lastP = pPa / 100.0f;
        iaq = lastIaq = (int)iaqF;
        iaqLabel = lastIaqLabel = bsecIAQLabel(iaqF);
        haveValidBme = true;
        Serial.print(F("BME680+BSEC — T: "));
        Serial.print(tBme, 1);
        Serial.print(F(" °C  RH: "));
        Serial.print(rhBme, 1);
        Serial.print(F(" %  P: "));
        Serial.print(p, 1);
        Serial.print(F(" hPa  IAQ: "));
        Serial.print(iaq);
        Serial.print(F(" ("));
        Serial.print(iaqLabel);
        Serial.println(F(")"));
      }
    }
  }

  if (hasScd41 || hasBme680) {
#if ENABLE_MQTT
    mqttConnect();
    mqttPublishState(co2, tScd, rhScd, tBme, rhBme, p, iaq);
#endif
    drawScreen(co2, tScd, rhScd, tBme, rhBme, p, iaq, iaqLabel, MEASURE_INTERVAL_SEC);
    cycleCount++;
    for (int s = MEASURE_INTERVAL_SEC - 1; s >= 0; s--) {
      if (hasBme680) bme680.run();
#if ENABLE_MQTT
      mqtt.loop();
      drawConnectionIndicator(WiFi.status() == WL_CONNECTED, mqtt.connected());
#endif
      drawCountdown(s);
      delay(1000);
    }
    if (hasBme680 && haveValidBme && (cycleCount % BSEC_STATE_SAVE_INTERVAL_CYCLES) == 0) {
      uint8_t stateBuf[BSEC_MAX_STATE_BLOB_SIZE];
      if (bme680.getState(stateBuf)) {
        Preferences prefs;
        if (prefs.begin("bsec", false)) {
          prefs.putBytes("state", stateBuf, BSEC_MAX_STATE_BLOB_SIZE);
          prefs.end();
        }
      }
    }
  } else {
    delay(5000);
  }
}
