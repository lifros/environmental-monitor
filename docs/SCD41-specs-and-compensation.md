# Sensirion SCD41 — Specifications and T/RH Compensation

Source: [SCD4x Datasheet](https://sensirion.com/media/documents/48C4B7FB/67FE0194/CD_DS_SCD4x_Datasheet_D1.pdf) (Section 1 and 3.7). Conditions: 25°C, 50% RH, 1013 mbar, periodic measurement, 3.3 V, unless stated.

---

## 1. SCD41 specifications (excerpt)

### CO2 (Table 1)
| Parameter | Conditions | Value |
|-----------|------------|--------|
| Output range | — | 0 – 40'000 ppm |
| Specified measurement range | — | 400 – 5'000 ppm |
| Accuracy (SCD41) | 400 – 1'000 ppm | ±(50 ppm + 2.5% of reading) |
| | 1'001 – 2'000 ppm | ±(50 ppm + 3% of reading) |
| | 2'001 – 5'000 ppm | ±(40 ppm + 5% of reading) |
| Repeatability | Typical | ±10 ppm |
| Response time | τ63%, step 400→2000 ppm | 60 s |

### Humidity (Table 2)
*“To achieve the specifications below, the temperature-offset inside the customer device must be set correctly (Section 3.7).”*

| Parameter | Conditions | Value |
|-----------|------------|--------|
| Range | — | 0 – 100 %RH |
| Accuracy (typ.) | 15–35°C, 20–65% RH | ±6 %RH |
| | -10–60°C, 0–100% RH | ±9 %RH |
| Repeatability | Typical | ±0.4 %RH |
| Response time | τ63%, periodic mode | 90 s |
| Drift | — | <0.25 %RH/year |

### Temperature (Table 3)
*Same note as humidity: temperature-offset must be set correctly for these specs.*

| Parameter | Conditions | Value |
|-----------|------------|--------|
| Range | — | -10 – 60 °C |
| Accuracy (typ.) | 15 – 35 °C | ±0.8 °C |
| | -10 – 60 °C | ±1.5 °C |
| Repeatability | — | ±0.1 °C |
| Response time | τ63%, periodic mode | 120 s |
| Drift | — | <0.03 °C/year |

### Electrical / interface
- I2C address: **0x62**
- Supply: 2.4 – 5.5 V (typ. 3.3 or 5 V)
- Average current (periodic, 1 meas/5 s): ~15–18 mA @ 3.3 V
- Commands: **set_temperature_offset** 0x241d, **get_temperature_offset** 0x2318 (only in **idle** mode)

---

## 2. How compensation works (Section 3.7)

- The SCD4x has **on-chip compensation**: it uses internal T and RH to correct the CO2 signal. **The temperature offset does not change CO2 accuracy**; it improves the **reported T and RH**.
- **Self-heating**: the device reads higher than ambient T; RH is then biased (warmer air “holds” more vapour). Setting the **temperature offset** lets the chip correct both **T and RH** in one go (no separate user formula for RH).
- **Default offset**: 4 °C (sensor reads ~4 °C above ambient out of the box).
- **Recommended offset range**: **0 °C to 20 °C**.

### Official formula (Equation 1 in datasheet)

\[
T_{offset\_actual} = T_{SCD4x} - T_{Reference} + T_{offset\_previous}
\]

- \(T_{SCD4x}\): current temperature **output** of the SCD4x (before changing offset).
- \(T_{Reference}\): reference (ambient) temperature, e.g. from a calibrated thermometer or another sensor (e.g. BME680 in same room).
- \(T_{offset\_previous}\): current offset in the sensor (read with **get_temperature_offset**; default 4 °C).

So: **offset = (sensor reading − reference) + previous_offset**. The chip then **subtracts** this offset from its internal reading to produce the reported T (and recalculates RH accordingly).

---

## 3. Effective compensation procedure

1. **Integrate** the SCD41 in the **final device** (same PCB, case, airflow as in use).
2. Run in **normal operation** (e.g. periodic measurement, same interval as application) until **thermal equilibrium** (e.g. 15–30 min).
3. Read **T from SCD4x** (e.g. 26.0 °C).
4. Measure **reference T** (ambient) with a calibrated thermometer or trusted sensor (e.g. BME680 away from heat sources) → e.g. 23.0 °C.
5. Read current offset: **get_temperature_offset** → e.g. 4.0 °C.
6. Compute:  
   \(T_{offset\_actual} = 26.0 - 23.0 + 4.0 = 7.0\) °C.
7. **set_temperature_offset(7.0)** (in **idle** mode: after **stop_periodic_measurement**, wait 500 ms; then set offset; then **start_periodic_measurement**).
8. (Optional) **persist_settings** so the offset survives power cycles (EEPROM limited to ~2000 writes).

In this project, `config.h` defines **SCD41_TEMP_OFFSET_C** (e.g. 2.5f). That value is the “sensor reads this much above ambient”; the firmware calls **setTemperatureOffset(SCD41_TEMP_OFFSET_C)** once at startup. Tune it (e.g. 2.0–4.0) by comparing SCD41 T with a reference; the chip will then report lower T and adjusted RH automatically.

---

## 4. Design-in (reduce self-heating)

- Place the SCD4x in the **coldest** part of the device (e.g. lowest, away from MCU/display/Wi‑Fi).
- **Maximum distance** from heat sources (CPU, display, regulators, batteries).
- Avoid mechanical coupling to vibrations; avoid direct, varying heat sources (the sensor only has **constant** offset correction).

---

## 5. Conversion of raw measurement (read_measurement)

From Table 11 (datasheet):

- **CO2** [ppm] = word[0]
- **T** [°C] = -45 + 175 × word[1] / (2^16 - 1)
- **RH** [%] = 100 × word[2] / (2^16 - 1)

Invalid or not-ready data often appear as **T = -45 °C**, **RH = 100 %** (raw 0 and 0xFFFF). The project treats such readings as invalid and keeps the last valid T/RH for display.
