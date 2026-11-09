#!/usr/bin/env bash
# Wwise-on-wine, following the Audiokinetic-Launcher-on-Linux guide.
#
# WHY A SEPARATE PREFIX: the game's prefix at /mnt/ssd/Cyberpunk 2077/game/prefix
# is never touched by anything but tools/deploy.sh (rule 2). dotnet48 + vcrun2022
# rewrite system DLLs inside a prefix; doing that to the game's prefix would risk
# the one install that currently boots. So Wwise gets its own.
set -euo pipefail
export WINEPREFIX=/mnt/sata/wwise/prefix
export WINEARCH=win64
export WINEDEBUG=-all
export W_OPT_UNATTENDED=1

echo "=== prefix: $WINEPREFIX"
mkdir -p "$WINEPREFIX"
wineboot -u
wineserver -w

for c in winhttp wininet xaudio29 vcrun2022 dotnet48; do
  echo "=== winetricks $c  ($(date +%H:%M:%S))"
  winetricks -q "$c" || { echo "!!! $c FAILED"; exit 1; }
  wineserver -w
done

echo "=== all components in  ($(date +%H:%M:%S))"
echo "=== next (needs a human, it opens a login window):"
echo "    WINEPREFIX=$WINEPREFIX wine /mnt/ssd/Downloads/AudiokineticLauncher-2026.1.1.6296.exe"
