# Sensirion SCD41 — Specifications and T/RH Compensation

Source: [SCD4x Datasheet](https://sensirion.com/media/documents/48C4B7FB/67FE0194/CD_DS_SCD4x_Datasheet_D1.pdf) (v1.7, Section 1 and 3). Conditions: 25°C, 50% RH, 1013 mbar, periodic measurement, 3.3 V, unless stated.

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

**Official correction for T:** The datasheet defines **only one** temperature correction: the **temperature offset** (command above). Recommended range: **0 °C to 20 °C**. There is no other T correction or software-offset in the SCD4x spec; the chip subtracts this offset from its internal reading and recalculates RH from it.

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
- \(T_{Reference}\): reference (ambient) temperature, e.g. from a calibrated thermometer or another sensor in the same environment.
- \(T_{offset\_previous}\): current offset in the sensor (read with **get_temperature_offset**; default 4 °C).

So: **offset = (sensor reading − reference) + previous_offset**. The chip then **subtracts** this offset from its internal reading to produce the reported T (and recalculates RH accordingly).

---

## 3. Effective compensation procedure

1. **Integrate** the SCD41 in the **final device** (same PCB, case, airflow as in use).
2. Run in **normal operation** (e.g. periodic measurement, same interval as application) until **thermal equilibrium** (e.g. 15–30 min).
3. Read **T from SCD4x** (e.g. 26.0 °C).
4. Measure **reference T** (ambient) with a calibrated thermometer or trusted sensor away from heat sources → e.g. 23.0 °C.
5. Read current offset: **get_temperature_offset** → e.g. 4.0 °C.
6. Compute:  
   \(T_{offset\_actual} = 26.0 - 23.0 + 4.0 = 7.0\) °C.
7. **set_temperature_offset(7.0)** (in **idle** mode: after **stop_periodic_measurement**, wait 500 ms; then set offset; then **start_periodic_measurement**).
8. (Optional) **persist_settings** so the offset survives power cycles (EEPROM limited to ~2000 writes).

In this project, `config.h` defines **SCD41_TEMP_OFFSET_C** (e.g. 3.0f). That value is the “sensor reads this much above ambient”; the firmware calls **setTemperatureOffset(SCD41_TEMP_OFFSET_C)** once at startup. Tune it (e.g. 2.0–4.0) by comparing SCD41 T with a reference; the chip will then report lower T and adjusted RH automatically.

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

---

## 6. Response time and reading interval

### CO2 response time (τ63% = 60 s)

The **response time** of 60 seconds (τ63%) means: after a step change in CO2 concentration (e.g., 400 → 2000 ppm), the sensor takes **~60 seconds to reach 63% of the final value**. To reach **95%** of the final value, approximately **3×τ63% ≈ 180 seconds** are needed.

**Implications for reading frequency:**

- **Internal update**: The sensor updates its internal measurement every **5 seconds** (periodic mode) or **~30 seconds** (low-power mode), but the **output value changes gradually** toward the new concentration.
- **Reading interval**: Reading every **60 seconds** (as in this project with `MEASURE_INTERVAL_SEC 60`) is **appropriate** because:
  - It matches the sensor’s response time (τ63%).
  - Reading more frequently (e.g., every 5 s) would show intermediate values that are still converging, not the stabilized reading.
  - Reading less frequently (e.g., every 180 s) would miss short-term changes but capture fully stabilized values.
- **For monitoring**: A 60-second interval provides a good balance: you see meaningful changes without excessive intermediate readings. For faster response to sudden changes, consider reading every 30–40 seconds, but expect gradual transitions.

**Note**: The response time depends on airflow, sensor placement, and the magnitude of the change. The 60 s value is typical for a step change 400→2000 ppm under standard conditions.

---

## 7. Pressure and altitude compensation (Section 3.7.3–3.7.6)

CO2 accuracy depends on ambient pressure. The SCD4x supports two options (use one; ambient pressure overrides altitude if both are set):

| Method | When to set | Range | Note |
|--------|-------------|--------|------|
| **set_sensor_altitude** | Once at installation, in **idle** | 0 – 3'000 m | Converts altitude to pressure internally. Default 0 m. Use **persist_settings** to save. |
| **set_ambient_pressure** | Can be sent **during** periodic measurement | 70'000 – 120'000 Pa | For dynamic pressure (e.g. from barometer). Default 101'300 Pa. Overrides altitude. |

- **Recommendation**: At fixed location, set altitude once (e.g. 200 m) and persist. For varying pressure (weather, travel), use **set_ambient_pressure** if you have a pressure source; otherwise altitude alone still improves accuracy vs default at sea level.

---

## 8. Field calibration: ASC and FRC (Section 3.8)

### Automatic Self-Calibration (ASC)

- **Default**: ASC is **enabled**. It keeps long-term accuracy without user action.
- **Condition**: The sensor must be exposed to air at **~400 ppm CO2** for **>3 minutes** at least **once per week** (with ≥4 h operation in periodic, low-power periodic, or single-shot 5 min mode). Typical: room with regular ventilation or occasional outdoor air.
- **ASC target**: Default 400 ppm; configurable (e.g. 435 ppm) via **set_automatic_self_calibration_target** if your “fresh air” baseline differs.
- **Warning**: *“Exposure to CO2 concentrations smaller than 400 ppm can affect the accuracy of the sensor with ASC enabled.”* In very clean environments (<400 ppm), consider disabling ASC or using FRC with a known reference.
- **persist_settings** stores ASC enabled/target in EEPROM (limited to ~2000 writes).

### Forced Recalibration (FRC)

- Use when you need a **one-time correction** (e.g. after assembly, or when ASC is not possible).
- **Procedure**:  
  1. Run the sensor in the **same mode** as normal use for **≥3 minutes** in **stable, known CO2** (e.g. outdoor 400 ppm).  
  2. **stop_periodic_measurement**, wait **500 ms**.  
  3. **perform_forced_recalibration**(reference_ppm), wait **400 ms**.  
  4. Return value **0xFFFF** = FRC failed (e.g. sensor not run before command).  
- Set altitude or ambient pressure **before** FRC if you use pressure compensation.

---

## 9. Persist settings (Section 3.10.1)

- Temperature offset, sensor altitude, ASC enabled/target are stored in **RAM** by default and lost at power cycle.
- **persist_settings** writes current configuration to **EEPROM** (max **2000** write cycles). Call only when you actually change settings and want them to survive power loss.
- **This project**: Offset (and optional altitude) are applied at every boot from `config.h`; **persist_settings** is optional (e.g. set `SCD41_PERSIST_SETTINGS 1` once after tuning, then set back to 0 to avoid EEPROM wear).

---

## 10. Measurement modes

| Mode | Command | Interval | Current (typ. 3.3 V) | Use case |
|------|---------|----------|----------------------|----------|
| **High performance** | start_periodic_measurement | 5 s | 15–18 mA | Fastest response, best for real-time monitoring. |
| **Low power** | start_low_power_periodic_measurement | ~30 s | ~3.2 mA | Battery or low power; ASC still works. |
| **Single shot** (SCD41/43) | measure_single_shot | On demand (min 5 s) | e.g. ~0.45 mA @ 1 meas/5 min | Lowest power, on-demand only; ASC tuned for 5 min interval. |

- **get_data_ready_status** can be used to poll before **read_measurement** (this project uses it).

---

## 11. Power supply (Section 2.3)

- **LDO** for the sensor is recommended; supply ripple (without sensor load) ≤ **30 mV** peak-to-peak.
- **Peak current**: up to ~205 mA @ 3.3 V during measurement; ensure the supply can deliver it without droop.

---

## 12. Summary: getting the best from the SCD41

1. **Temperature offset**: Set in final design under thermal equilibrium; use formula in Section 3; optionally persist.
2. **Altitude**: Set once if not at sea level (0–3000 m); optionally persist.
3. **Pressure**: Use **set_ambient_pressure** during measurement if you have a barometer and pressure varies.
4. **ASC**: Leave enabled if the device sees ~400 ppm at least weekly; otherwise consider FRC or disabling ASC.
5. **FRC**: Use for a quick fix with a known reference (e.g. outdoor 400 ppm, ≥3 min run before FRC).
6. **persist_settings**: Use sparingly (≤2000 times) after changing offset/altitude/ASC.
7. **Mode**: High-performance (5 s) for best responsiveness; low-power (30 s) to save power; single-shot for lowest power (SCD41 only).
8. **Design-in**: Place sensor away from heat; stable, low-ripple supply.

---

## 13. Best-practice checklist (this project)

| Item | Sensirion recommendation | This project |
|------|---------------------------|--------------|
| Config in idle | Set offset, altitude, ASC only when sensor is idle (after stop, 500 ms). | ✓ `stopPeriodicMeasurement()` → 500 ms → set offset/altitude → optional persist → 500 ms → start. |
| Temperature offset | 0–20 °C; use Equation 1 with reference T. | ✓ `SCD41_TEMP_OFFSET_C` in config; tune per installation. |
| Read flow | Use `get_data_ready_status` before `read_measurement` to avoid NACK. | ✓ Poll `getDataReadyStatus()` then `readMeasurement()`. |
| Invalid output | Treat T ≈ -45 °C, RH = 100 % as not-ready. | ✓ `scd41Valid()` rejects sentinel and enforces T -10..60 °C, RH 0..100 %. |
| ASC | Leave enabled if sensor sees ~400 ppm weekly. | ✓ ASC left at default (enabled). |
| Persist | Only when config must survive power cycle; EEPROM ≤2000 writes. | ✓ Optional `SCD41_PERSIST_SETTINGS`; default 0. |
| Altitude | Set once if not at sea level (0–3000 m). | ✓ Optional `SCD41_SENSOR_ALTITUDE_M`; 0 = not set. |
| Supply | LDO, ripple ≤30 mV; peak current ~205 mA. | Hardware; not enforced in firmware. |
