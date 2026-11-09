# External Sources Batch Conversion

Wwise allows headless batch conversion of external audio files (`.wav`) into Wwise game-ready audio formats (`.wem`, using Vorbis, ADPCM, Opus, or PCM) using `.wsources` XML files.

## XML Structure

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ExternalSourcesList SchemaVersion="1" Root="Z:\path\to\wav\files">
  <Source Path="sound.wav" Conversion="Vorbis Quality High" Destination="sound.wem"/>
</ExternalSourcesList>
```

> **Note on Paths**: In Wine, drive `Z:\` maps to the Linux root `/`. So `/mnt/sata/project` is reachable as `Z:\mnt\sata\project`.

## Running Conversion

```bash
# Using the unified CLI:
wwise convert examples/external_sources/sample.wsources

# Or directly through WwiseConsole:
wwise console convert-external-source "Z:\\path\\to\\sample.wsources"
```
