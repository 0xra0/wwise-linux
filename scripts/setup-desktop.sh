#!/usr/bin/env bash
# ==============================================================================
# Register Desktop Entry and URL Scheme Handlers
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_PATH="${SCRIPT_DIR}/bin/wwise"
APPS_DIR="${HOME}/.local/share/applications"

mkdir -p "$APPS_DIR"

echo "=== Registering Desktop Entries & URL Scheme Handlers ==="

# Generate desktop file pointing directly to this installation
DESKTOP_FILE="$APPS_DIR/wwise-launcher.desktop"

cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Name=Audiokinetic Wwise Launcher
Comment=Audiokinetic Wwise Launcher on Linux (Wine)
Exec=${BIN_PATH} url %u
Icon=audio-card
Terminal=false
Type=Application
Categories=Development;AudioVideo;Audio;
MimeType=x-scheme-handler/wwise-launcher;x-scheme-handler/ak-launcher;x-scheme-handler/ak-bug-reporter;
StartupWMClass=wwise launcher.exe
EOF

chmod +x "$DESKTOP_FILE"

# Authoring desktop entry
AUTHORING_DESKTOP="$APPS_DIR/wwise-authoring.desktop"
cat > "$AUTHORING_DESKTOP" <<EOF
[Desktop Entry]
Name=Audiokinetic Wwise Authoring
Comment=Wwise Sound Design & Interactive Audio Tool
Exec=${BIN_PATH} gui %f
Icon=audio-card
Terminal=false
Type=Application
Categories=Development;AudioVideo;Audio;
MimeType=application/x-wwise-project;
StartupWMClass=wwise.exe
EOF

chmod +x "$AUTHORING_DESKTOP"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APPS_DIR" 2>/dev/null || true
fi

# Associate custom URI schemes with our handler
if command -v xdg-mime >/dev/null 2>&1; then
    echo "Registering URL scheme handlers with xdg-mime..."
    xdg-mime default wwise-launcher.desktop x-scheme-handler/wwise-launcher
    xdg-mime default wwise-launcher.desktop x-scheme-handler/ak-launcher
    xdg-mime default wwise-launcher.desktop x-scheme-handler/ak-bug-reporter
    echo "Default handler: $(xdg-mime query default x-scheme-handler/wwise-launcher)"
fi

echo "Desktop integration registered successfully."
