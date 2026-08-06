list(APPEND ADD_INCLUDE "."
                        )

append_srcs_dir(ADD_SRCS "src"
                        )

# Most aggressive optimization for the benchmark code.
list(APPEND ADD_DEFINITIONS -Ofast
                            -funroll-all-loops
                            -fgcse-sm
                            -finline-limit=500
                            -fno-schedule-insns
                            -Wno-maybe-uninitialized
                            )

register_component()
