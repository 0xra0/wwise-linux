#!/usr/bin/env bash
# Runs the installed Wwise Launcher
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/bin/wwise" launcher "$@"
