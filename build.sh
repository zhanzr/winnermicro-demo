#!/usr/bin/env bash
#
# build.sh - WM IoT SDK build helper for bash (Git Bash / MSYS / Cygwin).
#
# Sets up the WinnerMicro CLI compile environment and forwards to wm.py.
# Run it from a project directory (or its build/ subdir - it auto-cds up),
# e.g.:
#
#   cd w806-nano/blink_hello
#   ../../build.sh build                  # configure + build
#   ../../build.sh flash -p COM36         # build + flash APP ONLY (fast)
#   ../../build.sh flash-full -p COM36    # build + full flash (pt + bootloader + app)
#   ../../build.sh monitor -p COM36       # serial monitor
#   ../../build.sh clean                  # clean build outputs
#
# It also works from inside the project's build/ directory (auto-detected):
#   cd w806-nano/blink_hello/build && ../../../build.sh build
#
# The bootloader and partition table are fixed SDK artifacts (identical for
# every project/board), so `flash` only sends the app image by default; use
# `flash-full` after a fresh board or an SDK update. An explicit -n/--name
# argument overrides the app-only default.
set -e

# Auto-cd to the project folder. This lets you run the script from inside
# the project's build/ dir (or a subdir) instead of the project root.
while [ ! -f CMakeLists.txt ] || [ ! -d main ]; do
    if [ "$PWD" = "/" ] || [ "$PWD" = "C:/" ] || [ "$PWD" = "C:\\" ]; then
        echo "error: run this script from a project folder (the one with CMakeLists.txt + main/)" >&2
        echo "       e.g.  cd hlk-w806-kit/blink_hello && ../../build.sh build" >&2
        exit 1
    fi
    cd ..
done

SDK="${WM_IOT_SDK_PATH:-/d/board_database/main-air101-w801/wm_iot_sdk}"
TOOL="${WM_CLI_PATH:-/d/winnermicro/wmclics}"

if [ ! -f "$SDK/tools/wm/wm.py" ]; then
    echo "error: WM_IOT_SDK_PATH not valid: $SDK" >&2
    exit 1
fi
if [ ! -x "$TOOL/python/python.exe" ]; then
    echo "error: WM CLI suite not found: $TOOL" >&2
    exit 1
fi

# Native Windows tools (ccache.exe) need the Windows USERPROFILE var.
# A proper MSYS2 login shell (the MSYS2 terminal) already provides it;
# a bare `bash.exe -c ...` does not.
if [ "$(uname -o 2>/dev/null)" = "Msys" ] && [ -z "$MSYSTEM" ]; then
    echo "warning: MSYSTEM is not set. Run this from a proper MSYS2 shell" >&2
    echo "         (the MSYS2 terminal, or with 'bash -l')." >&2
fi
if [ -z "$USERPROFILE" ]; then
    export USERPROFILE="$(cmd //c 'echo %USERPROFILE%' 2>/dev/null | tr -d '\r')"
fi

export WM_IOT_SDK_PATH="$SDK"
export PATH="$TOOL/ccache:$TOOL/cmake/bin:$TOOL/ninja:$TOOL/python:$TOOL/python/Scripts:$TOOL/toolchain/csky-elfabiv2-tools/bin:$PATH"
export PYTHONPATH="$TOOL/python/lib/site-packages"
export CCACHE_DIR="$TOOL/ccache/cache"

PY="$TOOL/python/python.exe"
WM_PY="$SDK/tools/wm/wm.py"

cmd="${1:-build}"
shift || true

# Does the user already pass an explicit -n/--name filter?
has_name=0
for a in "$@"; do
    if [ "$a" = "-n" ] || [ "$a" = "--name" ]; then has_name=1; fi
done

case "$cmd" in
    build|clean|distclean|monitor|menuconfig|set-soc)
        "$PY" "$WM_PY" "$cmd" "$@"
        ;;
    flash)
        if [ "$has_name" -eq 1 ]; then
            "$PY" "$WM_PY" flash "$@"
        else
            "$PY" "$WM_PY" flash -n app "$@"
        fi
        ;;
    flash-full)
        "$PY" "$WM_PY" flash "$@"
        ;;
    *)
        echo "usage: $0 <build|flash|flash-full|clean|distclean|monitor|menuconfig> [wm.py args...]" >&2
        exit 1
        ;;
esac
