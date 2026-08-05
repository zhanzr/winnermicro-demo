# Winner Micro W80x Projects

WM IoT SDK (`winnermicro/wm_iot_sdk`) projects for Winner Micro W80x
development boards.

This repository is organized **per board**: every board has its own directory
with its `README.md`, hardware notes, build/flash instructions and the
projects that run on it.

## Boards

| Board | Directory | SoC | CPU | Wireless |
|-------|-----------|-----|-----|----------|
| W800-nano (Air101-nano) | [`w800-nano/`](w800-nano/) | W800 (C-SKY XT804) | up to 240 MHz | none (RF not populated) |
| HLK-W801-KIT | [`hlk-w801-kit/`](hlk-w801-kit/) | W801 (C-SKY XT804) | up to 240 MHz | WiFi 4 + BLE 4.2 |

## Projects

Each board directory contains the projects built for that board. See the
board's `README.md` for details on building, flashing and expected output.

> The W800-nano / Air101-nano is an **MCU-only** board (no RF populated). The
> HLK-W801-KIT has the **full wireless stack** populated, so it is the board to
> use for Wi-Fi / BLE work.
