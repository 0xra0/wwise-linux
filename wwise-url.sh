#!/usr/bin/env bash
# Host-side OAuth URL protocol handler for wwise-launcher://
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/bin/wwise" url "$@"
