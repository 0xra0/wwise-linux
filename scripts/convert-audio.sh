#!/usr/bin/env bash
# ==============================================================================
# Helper tool to convert audio files (.wav) to Wwise format (.wem)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <sources.wsources or file.wav> [output_directory]"
    echo ""
    echo "Examples:"
    echo "  $0 my_sources.wsources"
    echo "  $0 audio/sfx.wav output/wem/"
    exit 1
fi

INPUT="$1"
OUTPUT_DIR="${2:-./wemout}"

if [[ "$INPUT" == *.wsources ]]; then
    exec "${SCRIPT_DIR}/bin/wwise" convert "$INPUT"
fi

if [[ "$INPUT" == *.wav ]]; then
    mkdir -p "$OUTPUT_DIR"
    TMP_WSOURCES="$(mktemp --suffix=.wsources)"
    ABS_IN="$(realpath "$INPUT")"
    IN_DIR="$(dirname "$ABS_IN")"
    IN_FILE="$(basename "$ABS_IN")"
    OUT_FILE="${IN_FILE%.wav}.wem"

    cat > "$TMP_WSOURCES" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<ExternalSourcesList SchemaVersion="1" Root="$(winepath -w "$IN_DIR")">
  <Source Path="$IN_FILE" Conversion="Vorbis Quality High" Destination="$OUT_FILE"/>
</ExternalSourcesList>
EOF

    echo "Converting $INPUT -> $OUTPUT_DIR/$OUT_FILE"
    "${SCRIPT_DIR}/bin/wwise" convert "$TMP_WSOURCES"
    rm -f "$TMP_WSOURCES"
    exit 0
fi

echo "Error: Unrecognized input format. Provide a .wsources or .wav file." >&2
exit 1
