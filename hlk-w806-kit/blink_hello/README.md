# HLK-W806-KIT — blink_hello

C-SKY XT804 @ 240 MHz

Blinks the three on-board LEDs and prints the system CPU frequency.

> Build / flash instructions: see the [board README](../README.md).

## Pins (LOW-active)

| LED | W806 pin | GPIO num |
|-----|----------|----------|
| LED1 | PB0 | `WM_GPIO_NUM_16` |
| LED2 | PB1 | `WM_GPIO_NUM_17` |
| LED3 | PB2 | `WM_GPIO_NUM_18` |

## Behavior

* Prints `System frequency: 240 MHz (240000000 Hz)` once at startup.
* Cycles the three LEDs (one at a time, 250 ms each) forever.
* Disables the hardware watchdog (not needed for this demo).

## Console output (UART0 @ 115200)

```
[I] (3) blink_hello: System frequency: 240 MHz (240000000 Hz)
[I] (4) blink_hello: LED on: PB0
[I] (254) blink_hello: LED on: PB1
[I] (504) blink_hello: LED on: PB2
...
```
