#!/usr/bin/env bash
# Repairs the override collision winetricks' `wininet` verb leaves behind, then
# finishes the guide's component list.
#
# WHAT WENT WRONG (measured, not guessed - winedump on both DLLs):
#   wine's builtin urlmon.dll forwards CreateUri BY NAME to iertutil.CreateUri.
#   Microsoft's Win7 iertutil.dll exports BY ORDINAL ONLY - it has no export
#   names at all. So iertutil=native breaks that forward, wine swaps in an
#   unimplemented-stub thunk, and every urlmon.CreateUri caller aborts.
#   vc_redist.x86.exe is one of them, which is why vcrun2022 died.
#
#   The mirror of it: native wininet.dll imports iertutil ORDINAL 650, and
#   wine's builtin iertutil stops at ordinal 9. That is the wineboot crash
#   (wineboot.c:1051 initialize_internet).
#
#   So the pair cannot be split, and cannot be native at prefix scope either:
#     native wininet  NEEDS native iertutil   (ordinal 650)
#     builtin urlmon  NEEDS builtin iertutil  (name forward)
#   Prefix scope goes builtin so the installers run. The launcher gets the
#   native pair TOGETHER, per-launch, from run-launcher.sh.
set -euo pipefail
export WINEPREFIX=/mnt/sata/wwise/prefix
export WINEARCH=win64
export WINEDEBUG=-all
export W_OPT_UNATTENDED=1

echo "=== $(date +%H:%M:%S) restoring builtin urlmon's forward target"
for d in wininet iertutil; do
  wine reg add 'HKCU\Software\Wine\DllOverrides' /v "*$d" /d builtin /f
done
wineserver -w

echo "=== $(date +%H:%M:%S) proving the fix before spending 20 minutes on it"
# vc_redist is the thing that failed; wineboot's initialize_internet is the
# other caller. If wineboot returns 0 with no iertutil/urlmon abort, both are
# resolved and the remaining verbs have a working loader.
# Not a pipeline: with `set -o pipefail` a grep-in-a-pipe conflates "wineboot
# failed" with "the abort message is absent", which are opposite verdicts.
wineboot -u > /mnt/sata/wwise/wineboot-check.log 2>&1 || true
if grep -iE 'unimplemented function (urlmon|iertutil)' /mnt/sata/wwise/wineboot-check.log; then
  echo "!!! still aborting - do not continue, the overrides did not take"
  exit 1
fi
wineserver -w
echo "=== $(date +%H:%M:%S) loader is clean"

for c in vcrun2022 dotnet48; do
  echo "=== winetricks $c  ($(date +%H:%M:%S))"
  winetricks -q "$c" || { echo "!!! $c FAILED"; exit 1; }
  wineserver -w
done
echo "=== all components in  ($(date +%H:%M:%S))"
echo "=== next: /mnt/sata/wwise/run-launcher.sh   (opens a login window, needs a human)"
