# W806-nano (Air103-nano) — dhry_240m

C-SKY XT804 @ 240 MHz

> Build / flash instructions: see the [board README](../README.md).

## Dhrystone 2.1

### Flags
```
-Ofast -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns
```

GCC 6.3.0 (csky-elfabiv2, T-HEAD C-SKY Tools V3.10.29)

### Result (240 MHz, 2,000,000 runs, `register` attribute)

Verbatim output (correctness checks all passed):

```
Dhrystone Benchmark, Version 2.1 (Language: C)

Program compiled with 'register' attribute

Execution starts, 2000000 runs through Dhrystone
Execution ends

Final values of the variables used in the benchmark:

Int_Glob:            5        should be:   5
Bool_Glob:           1        should be:   1
Ch_1_Glob:           A        should be:   A
Ch_2_Glob:           B        should be:   B
Arr_1_Glob[8]:       7        should be:   7
Arr_2_Glob[8][7]:    2000010  should be:   Number_Of_Runs + 10
Ptr_Glob->  Ptr_Comp:  (implementation-dependent)
  Discr:             0        should be:   0
  Enum_Comp:         2        should be:   2
  Int_Comp:          17       should be:   17
  Str_Comp:          DHRYSTONE PROGRAM, SOME STRING
Next_Ptr_Glob->  (same as above)
  Discr:             0        should be:   0
  Enum_Comp:         1        should be:   1
  Int_Comp:          18       should be:   18
Int_1_Loc:           5        should be:   5
Int_2_Loc:           13       should be:   13
Int_3_Loc:           7        should be:   7
Enum_Loc:            1        should be:   1

Microsecond for one run through Dhrystone[3-2934]:	 1.465
Dhrystones per Second:	682361.000
DMIPS/MHz:	1.618
```

### Summary

| Metric | Value |
|--------|-------|
| Microseconds per run | **1.465 µs** |
| Dhrystones per second | **682,361** |
| DMIPS/MHz | **1.618** |

## Notes

* CPU clock is set to 240 MHz via the project `device_table.toml`.
* 1 MB flash requires the project `partition_table_custom.csv` (see board README).
* The benchmark runs in a loop with a 10 s pause between runs.
* The watchdog is disabled at startup so the ~3 s run is not reset.
