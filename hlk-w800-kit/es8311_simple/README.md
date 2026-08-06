# HLK-W800-KIT — es8311_simple

Audio loop test using the on-board **ES8311** codec + ANT8108 speaker amp.

> Build / flash instructions: see the [board README](../README.md).

## What it does

1. Records ~3 s from the on-board mic (ES8311 ADC -> W800 I2S RX).
2. Plays the recorded sample back on the speaker (W800 I2S TX -> ES8311 DAC).
3. Plays a synthesized 400 -> 2000 Hz sweep.
4. Repeats forever.

## Wiring

| Signal | W800 pin | GPIO |
|--------|----------|------|
| CCLK (I2C clock) | PA1 | 1 |
| CDATA (I2C data) | PA4 | 4 |
| MCLK | PA7 | 7 |
| I2S_BCK | PB8 | 24 |
| I2S_LRCK | PB9 | 25 |
| I2S_DOUT | PB10 | 26 |
| I2S_DIN | PB11 | 27 |
| ANT8108 nShutdown | PB4 | 20 |

ES8311 I2C address: `0x18` (CE=0). Codec control is **bit-banged** on
PA1/PA4 (the W800 hardware I2C controller cannot reach this bus, see
`cht8305c_test`).

## ES8311 init

The register sequence in `main/src/main.c` is a direct port of the proven
Espressif `esp_codec_dev` es8311 driver (16 kHz, 16-bit, I2S standard,
slave mode, `use_mclk`, MCLK = 384 * fs = 6.144 MHz). W800 I2S is configured
with `wm_drv_i2s_set_mclk(6144000)` before `set_format(16000, 16-bit)`.

## Verified result

```
[I] es8311: ES8311 init complete
[I] es8311: ES8311 audio loop started
[I] es8311: recording 3 s...
[I] es8311: rx captured 96000/96000 bytes
[I] es8311: playing back recorded sample...
[I] es8311: tx played 96000 bytes
[I] es8311: playing sine sweep 400Hz->2000Hz
[I] es8311: tx played 96000 bytes
...
```

Full duplex audio data path verified over serial (`rx captured 96000/96000`,
`tx played 96000`). **Please confirm sound** — you should hear your voice
replayed after speaking, then a rising sweep.

## Notes

* I2S DMA reads/writes must be issued in chunks matching the ring buffer
  (`rx_pkt_size` 1200 B x 6); one large transfer never completes.
* `play_buffer()` sends the samples in 1200-byte chunks; the record loop
  captures in 1200-byte chunks too.
