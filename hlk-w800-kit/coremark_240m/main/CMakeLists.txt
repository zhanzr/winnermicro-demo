list(APPEND ADD_INCLUDE "."
                        )

append_srcs_dir(ADD_SRCS "src"
                        )

# Most aggressive optimization for the benchmark code.
# Note: the f4-demo (STM32F411) flag set (-Ofast -ffp-contract=fast
# -funroll-all-loops) measured 618.24 it/s on this board vs 660.73 with the
# flags below -- the csky extras are worth ~7%, keep them (2026-10-14 A/B).
list(APPEND ADD_DEFINITIONS -Ofast
                            -funroll-all-loops
                            -fgcse-sm
                            -finline-limit=500
                            -fno-schedule-insns
                            -Wno-maybe-uninitialized
                            )

register_component()
