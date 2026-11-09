## Wwise on Linux v1.0.0

A turnkey, production-ready environment and toolchain to run **Audiokinetic Wwise**, **Wwise Launcher**, **Authoring GUI**, and headless **WwiseConsole** natively on Linux via Wine.

### Highlights & Features

- **Full GUI Authoring**: Launch and use Wwise Authoring GUI on Linux with full sound engine playback and PulseAudio/PipeWire audio routing.
- **Wwise Launcher Support**: Complete Electron compatibility, enabling downloading, updating, and managing Wwise versions and SDK packages directly from Linux.
- **Host-to-Wine OAuth SSO**: Native XDG MIME protocol handler forwards `wwise-launcher://` browser redirects seamlessly back into the Wine Launcher instance.
- **Native C Compatibility Shims**:
  - `powershell_shim.c` (`powershell.exe`): Win32 Toolhelp32 process snapshot REPL handling `systeminformation` framing and process enumeration.
  - `cmdwait.c` (`cmd.exe`): ComSpec wrapper solving the detached `.lnk` race condition during elevated installation of prerequisites.
  - `uritest.c` (`uritest.exe`): Diagnostic utility validating `urlmon.dll` / `iertutil.dll` symbol forwarding.
- **Headless WwiseConsole CLI**:
  - Headless SoundBank generation for game builds.
  - External audio conversion (`.wav` -> `.wem` with Vorbis, ADPCM, Opus, PCM) via `.wsources`.
  - Batch project migration and verification.
- **WAAPI (Wwise Authoring API)**: Full WebSocket and HTTP WAAPI server operation on `127.0.0.1:8080`, allowing native Linux Python, Node.js, and game engine plugins to automate Wwise without bridging.
- **Automated One-Step Setup**: Includes `scripts/setup.sh` to provision the Wine prefix, apply DLL overrides, deploy shims, and configure desktop integrations.

### Included Prebuilt Binaries
- `shims/prebuilt/powershell.exe`
- `shims/prebuilt/cmd.exe` / `shims/prebuilt/cmdwait.exe`
- `shims/prebuilt/uritest.exe`

### Quick Start
```bash
git clone https://github.com/0xra/wwise-linux.git
cd wwise-linux
./scripts/setup.sh
./bin/wwise status
```
