# hlk-w800-kit

Hi-Link HLK-W800-KIT (W800 SoC, 240 MHz CK804EF). Control board for the W800-KIT-PRO
audio board (CHT8305C temp/humidity + ES8311 codec + ANT8108 speaker amp).

![HLK-W800-KIT board](board_0.png)
![HLK-W800-KIT board](board_1.png)

## Board facts
- **Chip**: W800 silicon -> `CONFIG_BUILD_TYPE_W800=y` in every `prj.config`.
- **USB**: Type-C -> **CH340N** (soldered; the CH340E on the schematic is **not populated**).
  - CH340N **RTS -> W800 RESET** (so RTS can reset the chip), **CTS <- W800 BOOT/PA0**
    (BOOT is read-only from the host; it cannot drive BOOT low).
- **UART**: 115200 8N1 on PA3 (TX) / PA2 (RX).
- **RGB LED**: PB0 = Green, PB1 = Red, PB2 = Blue (GPIO 16/17/18). **Active-HIGH**.
- **CHT8305C** (I2C, AD0=GND -> 7-bit addr `0x40`): **PA4 = SDA, PA1 = SCL**, ALERT = PB5 (GPIO 21, open-drain active-low).
- **ES8311 codec** (I2C addr `0x18`, CE=0): CCLK = PA1, MCLK = PA7, I2S_BCK = PB8,
  I2S_DOUT = PB10, I2S_LRCK = PB9, I2S_DIN = PB11 (user-confirmed).
- **ANT8108** speaker amp: nShutdown = PB4 (active low; drive high to enable).
- **Bootloader**: Hi-Link `boot2.0` (prints `Welcome boot2.0!`, emits `cccc` in download mode).

## Flashing (FIRST flash only)
The board ships with **valid factory firmware**, so on reset boot2.0 verifies and boots
the app instead of entering download mode. You must force BOOT low:

```bash
# in the project dir (e.g. hlk-w800-kit/blink_hello), in MSYS2 bash:
../../build.sh flash-full -p COM36
# at "please reset device": hold BOOT + press RST + release BOOT
```

This first `flash-full` installs the WinnerMicro `bootloader.img` **over boot2.0** plus
the app. After that the board behaves like the w806-nano: RTS->RESET + ESC auto-enters
download mode, so plain `../../build.sh flash -p COM36` works with no buttons.

## Projects
| Project | What it does |
|---|---|
| `blink_hello` | RGB LED cycle (Red -> Green -> Blue -> White -> OFF). |
| `dhry_240m` | Dhrystone 2.1 benchmark @ 240 MHz. |
| `coremark_240m` | CoreMark benchmark @ 240 MHz. |
| `wifi_con_test` | Connects to a given 2.4G AP (SSID/PSK in main.c). |
| `cht8305c_test` | Reads on-board CHT8305C temp/humidity every 1 s via **bit-bang I2C** (PA1=SCL, PA4=SDA, internal pull-ups). Result: temp=38.7 C, humi=58%RH, alert=1. |
| `es8311_simple` | Audio loop: record ~3 s → play back → 400-2000 Hz sweep. **Working** — ES8311 register init ported from Espressif `esp_codec_dev` es8311 driver (16 kHz, 16-bit, I2S, MCLK 384x). `rx captured 96000/96000` + `tx played 96000` verified over serial. |

## Build
```bash
cd hlk-w800-kit/<project>
../../build.sh build
```

## Notes
- **I2C quirk**: the W800 hardware I2C controller (device `i2c`, `fun2` mux) cannot
  reach the CHT8305C/ES8311 bus — every address NACKs. **Bit-bang I2C on the same
  pins (GPIO `fun5` + internal pull-ups) works perfectly.** Use the bit-bang
  implementation in `cht8305c_test/main/src/main.c` as the reference for all
  on-board I2C access.
- **I2S DMA chunks**: `wm_drv_i2s_read_async`/`write_async` must be called in chunks
  matching the ring buffer (`rx_pkt_size` x `rx_pkt_num`, e.g. 1200 B x 6); a single
  large transfer (e.g. 48000 B) never completes. `es8311_simple` shows the pattern.
- All builds are SDK artifacts; the bootloader/partition-table are byte-identical
  across projects (verified SHA256 `A412FE6...`), so app-only `flash` is fine after
  the first `flash-full`.
- Changing `prj.config` requires deleting `build/` (Kconfig cache).
