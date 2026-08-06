# HLK-W800-KIT — blink_hello

C-SKY XT804 @ 240 MHz

Cycles the on-board RGB LED and prints the system CPU frequency.

> Build / flash instructions: see the [board README](../README.md).

## Pins (all HIGH-active)

| Color | W800 pin | GPIO num |
|-------|----------|----------|
| Red   | PB1      | `WM_GPIO_NUM_17` |
| Green | PB0      | `WM_GPIO_NUM_16` |
| Blue  | PB2      | `WM_GPIO_NUM_18` |

## Behavior

* Prints `System frequency: 240 MHz (240000000 Hz)` once at startup.
* Cycles Red -> Green -> Blue -> White -> OFF (500 ms each) forever.
* Disables the hardware watchdog (not needed for this demo).

## Console output (UART0 @ 115200)

```
[I] (4) blink_hello: System frequency: 240 MHz (240000000 Hz)
[I] (4) blink_hello: LED: R
[I] (504) blink_hello: LED: G
[I] (1004) blink_hello: LED: B
[I] (1504) blink_hello: LED: RGB
[I] (2004) blink_hello: LED: off
...
```
