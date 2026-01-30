# Patch for GFX Library for Arduino (ESP32 core 3.x) — **optional**

This project now uses **Adafruit ST7789 + Adafruit GFX** for the display (ESP32 core 3.x compatible).  
You can remove `GFX_Library_for_Arduino` from the sketch.

If you still use that library elsewhere, apply the edits below in your Arduino **libraries** folder, inside  
`GFX_Library_for_Arduino\src\databus\`.

---

## 1. Arduino_ESP32SPI.cpp

**Line ~124** — change:
```cpp
_spi->dev->clock.val = spiFrequencyToClockDiv(old_apb / ((_spi->dev->clock.clkdiv_pre + 1) * (_spi->dev->clock.clkcnt_n + 1)));
```
to:
```cpp
_spi->dev->clock.val = spiFrequencyToClockDiv(_spi, old_apb / ((_spi->dev->clock.clkdiv_pre + 1) * (_spi->dev->clock.clkcnt_n + 1)));
```
(i.e. add `_spi, ` as first argument.)

**Line ~171** — change:
```cpp
_div = spiFrequencyToClockDiv(_speed);
```
to:
```cpp
_div = spiFrequencyToClockDiv(_spi, _speed);
```

---

## 2. Arduino_ESP32SPIDMA.cpp

**Line ~63** — change:
```cpp
_div = spiFrequencyToClockDiv(_speed);
```
to:
```cpp
_div = spiFrequencyToClockDiv(_spi, _speed);
```

---

After saving both files, recompile the sketch.  
You can then use either `Arduino_ESP32SPI` (hardware SPI) or keep `Arduino_SWSPI` in the sketch.
