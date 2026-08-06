# HLK-W806-KIT — coremark_240m

C-SKY XT804 @ 240 MHz

> Build / flash instructions: see the [board README](../README.md).

## CoreMark 1.0.1

### Flags
```
-Ofast -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns
```

GCC 6.3.0 (csky-elfabiv2, T-HEAD C-SKY Tools V3.10.29)

### Result (240 MHz, 7200 iterations, MEM_STATIC)

Verbatim output (correctness check passed):

```
2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 10891
Total time (secs): 10.891000
Iterations/Sec   : 661.096318
Iterations       : 7200
Compiler version : GCC 6.3.0
Compiler flags   : -Ofast -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0xbd59
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 661.096318 / GCC 6.3.0 -Ofast -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns / Static
Score (CoreMarks/MHz): 2.75
```

### Summary

| Metric | Value |
|--------|-------|
| Iterations | 7200 |
| Total time | 10.891 s |
| Iterations/Sec | **661.10** |
| CoreMark 1.0 score | **661.10** |
| CoreMarks/MHz | **2.75** |

## Notes

* CPU clock is set to 240 MHz via the project `device_table.toml`.
* 1 MB flash requires the project `partition_table_custom.csv` (see board README).
* 7200 iterations keeps the run above the 10 s minimum required by CoreMark.
* The benchmark runs in a loop with a 10 s pause between runs.
* The watchdog is disabled at startup so the ~11 s run is not reset.
