# Bosch BME680 — Specifications and Configuration Notes

Source: [BME680 Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme680-ds001.pdf) (BST-BME680-DS001, Rev 1.9). Values typ. at 25°C unless stated.

---

## 1. BME680 specifications (excerpt)

### Gas sensor (Table 2)
| Parameter | Condition | Value |
|-----------|-----------|--------|
| Operational range | — | -40–85°C, 10–95% r.H. |
| Heater current | 320°C, constant | 9–13 mA (VDD ≤ 1.8 V) |
| Response time τ33–63% | Brand-new sensors | Ultra-low: 92 s, Low power: 1.4 s, Continuous: 0.75 s |
| Resolution (gas resistance) | — | 0.05–0.11 % |
| Noise (gas resistance, RMS) | — | 1.5 % |

*IAQ index 0–500 and official classification (Excellent / Good / …) are defined for **BME680 + BSEC** (Bosch Software Environmental Cluster). Without BSEC, only raw gas resistance is available; our project maps it to a simple 0–500 scale via R_MIN/R_MAX.*

### Humidity (Table 6)
| Parameter | Condition | Value |
|-----------|-----------|--------|
| Operating range | — | -40–85°C, 0–100% r.H. |
| Full accuracy range | — | 0–65°C, 10–90% r.H. |
| Absolute accuracy | 20–80% r.H., 25°C, incl. hysteresis | ±3% r.H. |
| Hysteresis | 10→90→10% r.H., 25°C | ±1.5% r.H. |
| Response time τ0–63% | N2 → 90% r.H., 25°C | ~8 s |

### Pressure (Table 7)
| Parameter | Condition | Value |
|-----------|-----------|--------|
| Operating range | Full accuracy | 300–1100 hPa, 0–65°C |
| Absolute accuracy | 300–1100 hPa, 0–65°C | ±0.6 hPa |
| Temperature coefficient of offset | 25–40°C, 900 hPa | ±1.3 Pa/K (~10.9 cm/K) |
| Noise (RMS) | Highest oversampling, reduced BW | 0.2 Pa (~1.7 cm) |

### Temperature (Table 8)
| Parameter | Condition | Value |
|-----------|-----------|--------|
| Operating range | — | -40–85°C |
| Absolute accuracy | 25°C | ±0.5°C |
| | 0–65°C | ±1.0°C |

*Datasheet footnote: “Temperature measured by the internal temperature sensor … is **typically above ambient temperature**” (PCB and self-heating). There is **no software temperature-offset command** in the BME680 register map; BSEC compensates ambient T in their closed-source stack. The project optionally applies **software calibration** in firmware (*BME680_TEMP_OFFSET_C*, *BME680_HUMIDITY_OFFSET* in `config.h`): added to the raw reading, then RH clamped to 0–100%. This is user calibration to match a reference (e.g. another sensor); it does not change the sensor’s operating range or violate specs.*

---

## 2. Configuration (datasheet Section 3.2)

### Register order (Section 3.2.1)
Bosch recommends setting oversampling **in one write**, in this order: **osrs_h (humidity) → osrs_t (temperature) → osrs_p (pressure)**. The project sets them in that order in code.

### Quick start example (Section 3.2.1)
- Temperature oversampling: **2x**
- Pressure oversampling: **16x**
- Humidity oversampling: **1x**
- Gas heater: **300°C, 100 ms**

### Our project (environmental_monitor_gui / console)
- Temperature: **8x** (higher than quick start → lower noise)
- Humidity: **2x**
- Pressure: **4x** (16x would improve pressure noise; trade-off vs. measurement time)
- IIR filter: **size 3** (applies to **temperature and pressure** only; Section 3.3.4 — not to humidity or gas)
- Gas heater: **320°C, 150 ms** (within “typically 200–400°C”; datasheet Table 2 uses 320°C; 20–30 ms are enough to reach temperature, 150 ms is safe)

No change *required*; 320/150 is a common choice for indoor IAQ. Optionally, 300°C / 100 ms matches the quick start and slightly reduces power.

### Gas heater and status bits (Section 3.3.5, 3.4)
- Heater target: **200–400°C**; duration configurable (e.g. 1–4032 ms); **~20–30 ms** are needed for the heater to reach target.
- After readout, **heat_stab_r** (Section 5.3.5.6): if 0, heater did not stabilize (time too short or target too high) → gas reading may be unreliable.
- **gas_valid_r** (Section 5.3.5.5): indicates a valid gas conversion (vs. dummy slot).  
The Adafruit BME680 library does not expose these bits in the high-level API; for production, a custom read of the status register could be used to discard gas values when heat_stab_r is 0.

---

## 3. IAQ (0–500) without BSEC

- Official **IAQ index and bands** (0–50 Excellent, 51–100 Good, …) are defined for **BME680 + BSEC** (Section 4, Table 4).
- This project uses **raw gas resistance** and a **linear mapping** with **IAQ_R_MIN** and **IAQ_R_MAX** in `config.h`: lower resistance → worse IAQ, higher resistance → better IAQ. Tune R_MIN/R_MAX after 10–30 min burn-in for your environment.

---

## 4. Summary of possible improvements

| Item | Datasheet / note | Project action |
|------|-------------------|----------------|
| Oversampling order | H → T → P in one write | Set humidity first, then temperature, then pressure in code. |
| Pressure oversampling | Quick start uses 16x | We use 4x; 16x improves pressure noise, increases measurement time. |
| Heater 300 vs 320°C | Quick start 300°C 100 ms | We use 320°C 150 ms; both valid; 300/100 saves a little power. |
| heat_stab_r / gas_valid_r | Check for reliable gas reading | Not exposed by Adafruit API; optional custom read for production. |
| BME680 T above ambient | No offset in register map | Optional software offset in `config.h` (BME680_TEMP_OFFSET_C, BME680_HUMIDITY_OFFSET) to match a reference; RH clamped 0–100%. |
