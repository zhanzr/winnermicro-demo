# W806-nano (Air103-nano) — blink_hello

C-SKY XT804 @ 240 MHz

Blinks the three on-board LEDs and prints the system CPU frequency.

> Build / flash instructions: see the [board README](../README.md).

## Pins (HIGH-active)

| LED | W806 pin | GPIO num |
|-----|----------|----------|
| LED1 | PB24 | `WM_GPIO_NUM_40` |
| LED2 | PB25 | `WM_GPIO_NUM_41` |
| LED3 | PB26 | `WM_GPIO_NUM_42` |

Verified from the Air103 KiCad netlist: each LED's Pad1 → GND, Pad2 →
resistor → W806 GPIO (high-active).

## Behavior

* Prints `System frequency: 240 MHz (240000000 Hz)` once at startup.
* Cycles the three LEDs (one at a time, 250 ms each) forever.
* Disables the hardware watchdog (not needed for this demo).

## Console output (UART0 @ 115200)

```
[I] (3) blink_hello: System frequency: 240 MHz (240000000 Hz)
[I] (4) blink_hello: LED on: PB24
[I] (254) blink_hello: LED on: PB25
[I] (504) blink_hello: LED on: PB26
...
```
