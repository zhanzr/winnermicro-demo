# HLK-W801-KIT

The **HLK-W801-KIT** (Hi-Link / æµ·å‡Œç§?W801 development kit) is a full-featured
wireless board built around the **WinnerMicro W801** SoC. Unlike the
MCU-only Air101/W800-nano, the W801's **Wi-Fi/BLE RF is populated**, and the
W801 exposes **more GPIOs** (43 I/O) in its larger QFN56 package.

## Key facts

| Aspect | Value |
|--------|-------|
| SoC | WinnerMicro W801 (QFN56, 6Ã—6 mm) |
| CPU | 32-bit C-SKY XT804, up to **240 MHz** (`-mcpu=ck804ef`) |
| Memory | 2 MB SPI flash, 288 KB RAM |
| Wireless | **Wi-Fi 4 (802.11 b/g/n) + BLE 4.2 â€?populated** |
| GPIOs | 43 (PA0-15, PB0-27 minus PB23) |
| USB bridge | CH340 (Type-C), **115200 8N1** |
| Console | UART0 on PB19(TX) / PB20(RX) |
| Download mode | CH340 DTR/RTS auto-reset into W80x secboot, XMODEM (`wm.py flash`) |
| LEDs | **7 on-board LEDs, all LOW-active** |

## Memory (RAM) â€?full 288 KB is usable in pure C

Same W80x silicon as the W800: **288 KB RAM** = **160 KB SRAM**
(`0x20000000`) + **128 KB DRAM** (`0x20028000`), both exposed to the C heap
in the WM IoT SDK. Measured free heap at boot (blink_hello): `272688
(266.30 KB)`; with WiFi enabled (wifi_con_test): `265000 (258.79 KB)`.

## Board photo

![HLK-W801-KIT board](board_0.png)

> **SDK target note:** the WM IoT SDK's chip choice list has `W801` commented
> out; the 43-GPIO W80x silicon is built with the **`W801S`** config
> (`CONFIG_BUILD_TYPE_W801S=y`), which is the SDK's name for this silicon.

## LEDs (all LOW-active: GPIO low = LED on)

| LED | W801 pin | GPIO num |
|-----|----------|----------|
| LED1 | PB25 | `WM_GPIO_NUM_41` |
| LED2 | PB26 | `WM_GPIO_NUM_42` |
| LED3 | PB18 | `WM_GPIO_NUM_34` |
| LED4 | PB16 | `WM_GPIO_NUM_32` |
| LED5 | PB17 | `WM_GPIO_NUM_33` |
| LED6 | PB11 | `WM_GPIO_NUM_27` |
| LED7 | PB5  | `WM_GPIO_NUM_21` |

```c
wm_drv_gpio_data_reset(pin); /* LED on  */
wm_drv_gpio_data_set(pin);   /* LED off */
```

## Projects

| Project | Purpose |
|---------|---------|
| [`blink_hello`](blink_hello/) | Blink the 7 LEDs + print system frequency |
| [`dhry_240m`](dhry_240m/) | Dhrystone 2.1 @ 240 MHz |
| [`coremark_240m`](coremark_240m/) | CoreMark 1.0.1 @ 240 MHz |
| [`wifi_con_test`](wifi_con_test/) | WiFi scan + connect test (full wireless) |

### Measured results (GCC 6.3.0, 240 MHz)

**`dhry_240m`** (2,000,000 runs, `register` attribute):

```
Microseconds for one run through Dhrystone: 1.494
Dhrystones per Second:                      669120.1
Score (DMIPS/MHz):                          1.587
```

**`coremark_240m`** (7200 iterations):

```
Iterations/Sec   : 660.73
CoreMark 1.0 : 660.732312 / GCC 6.3.0 -Ofast -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns / Static
Score (CoreMarks/MHz): 2.75
```

**`wifi_con_test`** (verified against a real AP):

```
--- Scanning for available APs ---
  Found 3 AP(s)
  [00] HIKVISION_AE4490_2G4   -58 dBm  ch 8  WPA/WPA2
  ...
--- Connecting to "HIKVISION_AE4490_2G4" (WPA2, 30s timeout) ---
WiFi: connected to AP
WiFi: got IP 192.168.5.253
  LINK_UP
Connected successfully
  IP: 192.168.5.253
--- WiFi status: UP ("HIKVISION_AE4490_2G4", RSSI: -58 dBm) ---
```

## Dependencies

- **WM IoT SDK** (CMake-based build system, `tools/wm/wm.py`).
  On this machine: `D:\board_database\main-air101-w801\wm_iot_sdk`
  (set as the `WM_IOT_SDK_PATH` environment variable).
- **WinnerMicro CLI Compile Suite**: bundled C-SKY toolchain
  (`csky-elfabiv2-gcc`, T-HEAD C-SKY Tools V3.10.29 / GCC 6.3.0), CMake,
  Ninja, ccache and Python. On this machine: `D:\winnermicro\wmclics`.
- CH340 USB-UART driver (board console / flashing bridge).

## Building and flashing

Every build must run with the CLI compile suite's tools on `PATH` so that
`cmake`, `ninja`, `python` and the C-SKY toolchain resolve correctly.

**Quick way (bash / Git Bash / MSYS):**

```bash
cd hlk-w801-kit/blink_hello
../../build.sh build                 # configure + build
../../build.sh flash -p COM36         # build + flash APP ONLY (fast)
../../build.sh flash-full -p COM36    # build + FULL flash (pt + bootloader + app)
../../build.sh monitor -p COM36      # serial monitor
```

**Manual way (PowerShell):**

```powershell
# 1. Prepare the environment (once per shell)
$env:WM_IOT_SDK_PATH = "D:\board_database\main-air101-w801\wm_iot_sdk"
$tool = "D:\winnermicro\wmclics"
$env:PATH = "$tool;$tool\ccache;$tool\cmake\bin;$tool\ninja;$tool\python;$tool\python\Scripts;$tool\toolchain\csky-elfabiv2-tools\bin;$env:PATH"
$env:PYTHONPATH = "$tool\python\lib\site-packages"

# 2. Build a project
cd hlk-w801-kit\blink_hello
python $env:WM_IOT_SDK_PATH\tools\wm\wm.py build

# 3. Flash to the board (substitute the CH340 COM port)
python $env:WM_IOT_SDK_PATH\tools\wm\wm.py flash -p COM36

# 4. Watch the console output
python $env:WM_IOT_SDK_PATH\tools\wm\wm.py monitor -p COM36
```

Notes:

- The CH340 enumerates as a COM port; with multiple USB-serial devices you
  must pass the right one to `-p`. Confirm the port by resetting the board and
  watching for the W80x `Secboot V0.6` + `C` (XMODEM) handshake.
- `wm.py flash` enters secboot download mode through the CH340's DTR/RTS
  reset line and transfers firmware over XMODEM. If the board does not
  auto-sync, hold **BOOT** while pressing **RESET**.
- Benchmark projects disable the hardware watchdog at startup and loop with a
  10 s pause between runs.
- `wifi_con_test` reads the target AP from `main/src/wifi_config.h`
  (gitignored; see `wifi_config.h.example`). Fill in your SSID/password and
  rebuild.
