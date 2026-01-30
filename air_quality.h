/**
 * Indoor air quality estimate from CO2, VOC (gas resistance), temperature, humidity.
 * Score 0–100 and category. Uses simple bands; tune thresholds for your environment.
 */
#ifndef AIR_QUALITY_H
#define AIR_QUALITY_H

#include <Arduino.h>

// Input: use SCD41 for CO2/T/RH when available, BME680 for gas and backup T/RH
struct AirQualityInput {
  bool has_co2;
  float co2_ppm;
  float temperature_c;   // prefer SCD41 or average
  float humidity_percent;
  float pressure_hpa;
  uint32_t gas_resistance_ohm;  // BME680: higher = cleaner air
};

// Output
struct AirQualityEstimate {
  uint8_t score;           // 0–100
  const char* category;     // "Excellent", "Good", "Moderate", "Poor", "Bad"
  const char* suggestion;   // short tip (e.g. "Ventilate")
};

// Thresholds (tune for your use case)
#define AQ_CO2_EXCELLENT   500
#define AQ_CO2_GOOD        800
#define AQ_CO2_MODERATE    1200
#define AQ_CO2_POOR        2000
#define AQ_GAS_GOOD        150000
#define AQ_GAS_MODERATE     80000
#define AQ_GAS_POOR         30000
#define AQ_RH_LOW           25
#define AQ_RH_HIGH          65
#define AQ_T_COLD           16
#define AQ_T_HOT            28

inline AirQualityEstimate estimateAirQuality(const AirQualityInput& in) {
  AirQualityEstimate out = { 0, "Unknown", "" };
  float co2Score = 100.0f;
  float gasScore = 100.0f;
  float comfortScore = 100.0f;

  // CO2 score (100 = best at low ppm, 0 = very high)
  if (in.has_co2 && in.co2_ppm > 0) {
    if (in.co2_ppm <= AQ_CO2_EXCELLENT)
      co2Score = 100.0f;
    else if (in.co2_ppm <= AQ_CO2_GOOD)
      co2Score = 100.0f - 30.0f * (in.co2_ppm - AQ_CO2_EXCELLENT) / (float)(AQ_CO2_GOOD - AQ_CO2_EXCELLENT);
    else if (in.co2_ppm <= AQ_CO2_MODERATE)
      co2Score = 70.0f - 30.0f * (in.co2_ppm - AQ_CO2_GOOD) / (float)(AQ_CO2_MODERATE - AQ_CO2_GOOD);
    else if (in.co2_ppm <= AQ_CO2_POOR)
      co2Score = 40.0f - 30.0f * (in.co2_ppm - AQ_CO2_MODERATE) / (float)(AQ_CO2_POOR - AQ_CO2_MODERATE);
    else
      co2Score = 10.0f;
    if (co2Score < 0) co2Score = 0;
  }

  // VOC proxy from BME680 gas resistance (higher = cleaner)
  if (in.gas_resistance_ohm > 0) {
    if (in.gas_resistance_ohm >= AQ_GAS_GOOD)
      gasScore = 100.0f;
    else if (in.gas_resistance_ohm >= AQ_GAS_MODERATE)
      gasScore = 60.0f + 40.0f * (in.gas_resistance_ohm - AQ_GAS_MODERATE) / (float)(AQ_GAS_GOOD - AQ_GAS_MODERATE);
    else if (in.gas_resistance_ohm >= AQ_GAS_POOR)
      gasScore = 30.0f + 30.0f * (in.gas_resistance_ohm - AQ_GAS_POOR) / (float)(AQ_GAS_MODERATE - AQ_GAS_POOR);
    else
      gasScore = 30.0f * (float)in.gas_resistance_ohm / (float)AQ_GAS_POOR;
    if (gasScore > 100) gasScore = 100;
    if (gasScore < 0) gasScore = 0;
  }

  // Comfort: humidity and temperature (small penalty if outside ideal range)
  if (in.humidity_percent > 0) {
    if (in.humidity_percent < AQ_RH_LOW)
      comfortScore -= 15;
    else if (in.humidity_percent > AQ_RH_HIGH)
      comfortScore -= 15;
  }
  if (in.temperature_c != 0 || in.has_co2) {
    if (in.temperature_c < AQ_T_COLD)
      comfortScore -= 10;
    else if (in.temperature_c > AQ_T_HOT)
      comfortScore -= 10;
  }
  if (comfortScore < 0) comfortScore = 0;

  // Combined: weight CO2 and gas heavily, comfort slightly
  float total;
  if (in.has_co2 && in.gas_resistance_ohm > 0)
    total = 0.45f * co2Score + 0.45f * gasScore + 0.1f * comfortScore;
  else if (in.has_co2)
    total = 0.85f * co2Score + 0.15f * comfortScore;
  else if (in.gas_resistance_ohm > 0)
    total = 0.85f * gasScore + 0.15f * comfortScore;
  else
    total = comfortScore;

  if (total > 100) total = 100;
  out.score = (uint8_t)(total + 0.5f);

  // Category and suggestion
  if (out.score >= 80) {
    out.category = "Excellent";
    out.suggestion = "Air quality is good.";
  } else if (out.score >= 60) {
    out.category = "Good";
    out.suggestion = "Minor improvement possible.";
  } else if (out.score >= 40) {
    out.category = "Moderate";
    out.suggestion = "Ventilate or reduce sources.";
  } else if (out.score >= 20) {
    out.category = "Poor";
    out.suggestion = "Ventilate the room.";
  } else {
    out.category = "Bad";
    out.suggestion = "Open windows; avoid prolonged stay.";
  }

  return out;
}

#endif // AIR_QUALITY_H
