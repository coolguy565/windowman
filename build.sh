#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"

# Try MinGW cross-compiler first (available in WSL)
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    echo "Building with MinGW-w64 cross-compiler..."
    x86_64-w64-mingw32-gcc -O2 -Wall -municode -mwindows -o "$DIR/WindowMan.exe" "$DIR/main.c" -luser32 -lshell32 -lole32 -luuid -lshlwapi
    echo "Build successful: $DIR/WindowMan.exe"
else
    # Fall back to MSVC via WSL interop
    echo "Building with MSVC via WSL interop..."
    WIN_DIR="$(wslpath -w "$DIR")"
    cmd.exe /c "cd /d $WIN_DIR && build.bat" 2>&1
fi
