#!/usr/bin/env bash
# ==============================================================================
# Setup & Deploy Shims to Wine Prefix
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export WINEPREFIX="${WINEPREFIX:-${SCRIPT_DIR}/prefix}"
export WINEARCH="${WINEARCH:-win64}"

echo "=== Setting up Wwise Linux shims in $WINEPREFIX ==="

# Check for compiler or prebuilt binaries
SHIM_DIR="${SCRIPT_DIR}/shims"
mkdir -p "$WINEPREFIX/drive_c/windows/system32"
mkdir -p "$WINEPREFIX/drive_c/windows/cmdwait"

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    echo "Compiling shims from source with MinGW-w64..."
    make -C "$SHIM_DIR" all
    cp -f "$SHIM_DIR/powershell.exe" "$WINEPREFIX/drive_c/windows/system32/powershell.exe"
    cp -f "$SHIM_DIR/cmd.exe" "$WINEPREFIX/drive_c/windows/cmdwait/cmd.exe"
else
    echo "MinGW-w64 not detected; deploying prebuilt binaries..."
    if [ ! -f "$SHIM_DIR/prebuilt/powershell.exe" ] || [ ! -f "$SHIM_DIR/prebuilt/cmd.exe" ]; then
        echo "Error: Prebuilt binaries not found in $SHIM_DIR/prebuilt/" >&2
        exit 1
    fi
    cp -f "$SHIM_DIR/prebuilt/powershell.exe" "$WINEPREFIX/drive_c/windows/system32/powershell.exe"
    cp -f "$SHIM_DIR/prebuilt/cmd.exe" "$WINEPREFIX/drive_c/windows/cmdwait/cmd.exe"
fi

echo "Registering ComSpec in Wine registry..."
wine reg add 'HKLM\System\CurrentControlSet\Control\Session Manager\Environment' \
    /v "ComSpec" /t REG_EXPAND_SZ /d '%SystemRoot%\cmdwait\cmd.exe' /f
wineserver -w

echo "Shims installed successfully:"
echo "  - $WINEPREFIX/drive_c/windows/system32/powershell.exe"
echo "  - $WINEPREFIX/drive_c/windows/cmdwait/cmd.exe (ComSpec)"
