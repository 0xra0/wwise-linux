#!/usr/bin/env bash
# Headless WwiseConsole runner
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/bin/wwise" console "$@"
