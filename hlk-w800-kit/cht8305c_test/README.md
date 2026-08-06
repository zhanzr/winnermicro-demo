# HLK-W800-KIT — cht8305c_test

Reads the on-board **CHT8305C** temperature / relative humidity sensor.

> Build / flash instructions: see the [board README](../README.md).

## How it reads the sensor

* The W800 hardware I2C controller **cannot** reach the on-board bus (every
  address NACKs), so this project **bit-bangs I2C** on PA1 (SCL) / PA4 (SDA)
  as plain GPIO (`fun5`) with the internal pull-ups enabled.
* CHT8305C protocol: write register `0x00` (trigger), wait ~20 ms, then read
  4 bytes = temp MSB, temp LSB, humi MSB, humi LSB.
* Formula: `T(C) = 165 * raw/65535 - 40` ; `RH(%) = 100 * raw/65535`.
* ALERT (PB5, open-drain active-low) is polled too.

## Verified result

```
[I] cht8305c: CHT8305C test started (SCL=PA1, SDA=PA4)
[I] cht8305c: temp=38.74 C, humi=58.43 %RH, alert=1
[I] cht8305c: temp=38.76 C, humi=57.93 %RH, alert=1
[I] cht8305c: temp=39.42 C, humi=56.04 %RH, alert=1
...
```

* Temperature settled around **39 °C** (sensor sits next to the running W800;
  reads a couple of degrees above ambient), humidity ~**56-58 %RH**, drifting
  as the board warmed up.
* `alert=1` = no alert pending (normal).

## Notes

* The sensor's I2C address is `0x40` (7-bit) with AD0 tied to GND. The probe
  loop also checks `0x44/0x48/0x4C` (other AD0 strappings) and reports which
  one ACKs.
* The bit-bang I2C helpers here are the reference implementation for all
  on-board I2C access (also reused by `es8311_simple`).
