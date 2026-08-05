# HLK-W801-KIT — blink_hello

C-SKY XT804 @ 240 MHz

Blinks the seven on-board LEDs and prints the system CPU frequency.

> Build / flash instructions: see the [board README](../README.md).

## Pins (all LOW-active)

| LED | W801 pin | GPIO num |
|-----|----------|----------|
| LED1 | PB25 | `WM_GPIO_NUM_41` |
| LED2 | PB26 | `WM_GPIO_NUM_42` |
| LED3 | PB18 | `WM_GPIO_NUM_34` |
| LED4 | PB16 | `WM_GPIO_NUM_32` |
| LED5 | PB17 | `WM_GPIO_NUM_33` |
| LED6 | PB11 | `WM_GPIO_NUM_27` |
| LED7 | PB5  | `WM_GPIO_NUM_21` |

## Behavior

* Prints `System frequency: 240 MHz (240000000 Hz)` once at startup.
* Cycles the seven LEDs (one at a time, 150 ms each) forever.
* Disables the hardware watchdog (not needed for this demo).

## Console output (UART0 @ 115200)

```
[I] (4) blink_hello: System frequency: 240 MHz (240000000 Hz)
[I] (4) blink_hello: LED on: LED1 (PB25)
[I] (154) blink_hello: LED on: LED2 (PB26)
[I] (304) blink_hello: LED on: LED3 (PB18)
...
```
