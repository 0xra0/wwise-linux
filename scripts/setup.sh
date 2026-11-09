#!/usr/bin/env bash
# ==============================================================================
# Complete Automated Setup for Wwise on Linux
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export WINEPREFIX="${WINEPREFIX:-${SCRIPT_DIR}/prefix}"
export WINEARCH="${WINEARCH:-win64}"

cat <<'EOF'
  _       ___      _               ___        __ _                  
 | |  _  / (_)___ (_)__   ___     / _ \ _ _  / /(_)_ _ _  ___ _____ 
 | |/ // /_/ (_-< / / -_)(_-<    / // // ' \/ / | | ' \ || \ \ / -_)
 |___//___/_//__//_/\__//___/    \___//_||_/_/  |_|_||_\_,_/_\_\___|
========================================================================
 Automated Installation & Configuration for Linux
========================================================================
EOF

echo "Checking host dependencies..."
MISSING_DEPS=()
for dep in wine winetricks; do
    if ! command -v "$dep" >/dev/null 2>&1; then
        MISSING_DEPS+=("$dep")
    fi
done

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "Error: Missing required host tools: ${MISSING_DEPS[*]}" >&2
    echo "Please install them via your distribution package manager." >&2
    echo "  Ubuntu/Debian: sudo apt install wine winetricks mingw-w64" >&2
    echo "  Arch Linux:    sudo pacman -S wine winetricks mingw-w64-gcc" >&2
    echo "  Fedora:        sudo dnf install wine winetricks mingw64-gcc" >&2
    exit 1
fi

echo "Wine version: $(wine --version)"

# 1. Initialize Prefix & Winetricks
if [ -d "$WINEPREFIX/drive_c/windows" ]; then
    echo "Existing prefix detected at: $WINEPREFIX"
else
    echo "Creating and provisioning Wine prefix at: $WINEPREFIX..."
    "${SCRIPT_DIR}/scripts/setup-prefix.sh"
fi

# 2. Deploy Shims
echo "Deploying custom compatibility shims (PowerShell, CmdWait)..."
"${SCRIPT_DIR}/scripts/setup-shims.sh"

# 3. Setup Desktop & MIME URL Handlers
echo "Configuring desktop integration and OAuth URL handler..."
"${SCRIPT_DIR}/scripts/setup-desktop.sh"

echo ""
echo "========================================================================"
echo " Setup Complete!"
echo "========================================================================"
echo "Next Steps:"
echo "1. Download the Audiokinetic Launcher from:"
echo "   https://www.audiokinetic.com/en/download"
echo ""
echo "2. Install the Launcher using:"
echo "   ${SCRIPT_DIR}/bin/wwise install-launcher <path/to/AudiokineticLauncher.exe>"
echo ""
echo "3. Run the installed Launcher and sign in:"
echo "   ${SCRIPT_DIR}/bin/wwise launcher"
echo ""
echo "4. Launch Wwise Authoring GUI once installed:"
echo "   ${SCRIPT_DIR}/bin/wwise gui"
echo ""
echo "5. Check status anytime with:"
echo "   ${SCRIPT_DIR}/bin/wwise status"
echo "========================================================================"
