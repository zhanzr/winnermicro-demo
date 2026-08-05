# W800-nano (Air101-nano) — blink_hello

C-SKY XT804 @ 240 MHz

Blinks the three on-board LEDs and prints the system CPU frequency.

> Build / flash instructions: see the [board README](../README.md).

## Pins

| LED | W800 pin | GPIO num |
|-----|----------|----------|
| LED1 | PB8  | `WM_GPIO_NUM_24` |
| LED2 | PB9  | `WM_GPIO_NUM_25` |
| LED3 | PB10 | `WM_GPIO_NUM_26` |

All three LEDs are wired **high-active** (GPIO → 5.1kΩ resistor → LED anode,
LED cathode → GND), so a high output turns the LED on. Verified from the
KiCad netlist: each LED's Pad1 → GND, Pad2 → resistor → W800 GPIO.

## Behavior

* Prints `System frequency: 240 MHz (240000000 Hz)` once at startup.
* Cycles the three LEDs (one at a time, 250 ms each) forever.
* Disables the hardware watchdog (not needed for this demo).

## Console output (UART0 @ 115200)

```
[I] (4) blink_hello: System frequency: 240 MHz (240000000 Hz)
[I] (4) blink_hello: LED on: PB8
[I] (254) blink_hello: LED on: PB9
[I] (504) blink_hello: LED on: PB10
...
```
