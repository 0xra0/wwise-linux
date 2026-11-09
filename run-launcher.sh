#!/usr/bin/env bash
# Audiokinetic Launcher installer runner (with native network stack scoped to this process)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/bin/wwise" install-launcher "$@"
