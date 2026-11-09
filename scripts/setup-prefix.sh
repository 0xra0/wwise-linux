#!/usr/bin/env bash
# ==============================================================================
# Wwise Wine Prefix Setup
# ==============================================================================
# Creates an isolated 64-bit Wine prefix and installs baseline components.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export WINEPREFIX="${WINEPREFIX:-${SCRIPT_DIR}/prefix}"
export WINEARCH="${WINEARCH:-win64}"
export WINEDEBUG="${WINEDEBUG:--all}"
export W_OPT_UNATTENDED=1

echo "=== Initializing Wine Prefix at: $WINEPREFIX ==="
mkdir -p "$WINEPREFIX"
wineboot -u
wineserver -w

# Step 1: Install prerequisites
for c in winhttp wininet xaudio29; do
    echo "=== winetricks $c ($(date +%H:%M:%S)) ==="
    winetricks -q "$c" || { echo "Error: winetricks $c failed" >&2; exit 1; }
    wineserver -w
done

# Step 2: Apply the critical wininet/iertutil repair
echo "=== Applying DllOverrides repair for urlmon/iertutil ==="
for d in wininet iertutil; do
    wine reg add 'HKCU\Software\Wine\DllOverrides' /v "*$d" /d builtin /f
done
wineserver -w

# Step 3: Install vcrun2022 and dotnet48 with clean loader
for c in vcrun2022 dotnet48; do
    echo "=== winetricks $c ($(date +%H:%M:%S)) ==="
    winetricks -q "$c" || { echo "Error: winetricks $c failed" >&2; exit 1; }
    wineserver -w
done

echo "=== Wine prefix components installed successfully ($(date +%H:%M:%S)) ==="
