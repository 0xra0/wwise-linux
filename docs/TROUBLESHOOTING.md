# Troubleshooting Wwise on Linux

This guide covers common issues, audio driver quirks, and debugging steps when running Wwise on Linux.

---

## 1. Diagnostics & Status Check

Always start by running the built-in diagnostic status tool:
```bash
./bin/wwise status
```

This verifies:
- Wine prefix existence
- Wine version
- Wwise Launcher and Wwise Authoring executables
- PowerShell and CmdWait shims deployment
- Desktop MIME scheme registrations

---

## 2. Common Issues & Solutions

### Launcher exits immediately with "Failed to retrieve running processes"
- **Cause**: Wine's built-in `powershell.exe` stub is active instead of the compatibility shim.
- **Fix**: Run:
  ```bash
  ./scripts/setup-shims.sh
  ```
  Ensure `WINEDLLOVERRIDES="powershell.exe=n"` is set when starting the application (handled automatically by `bin/wwise`).

### Launcher fails when clicking "Sign In" / browser never opens
- **Cause**: Native `iertutil.dll` is active, breaking `urlmon.CreateUri` forwarding.
- **Fix**: Run:
  ```bash
  ./bin/wwise test-uri
  ```
  If it reports error 127, reset the DLL override in the prefix:
  ```bash
  wine reg add 'HKCU\Software\Wine\DllOverrides' /v "*iertutil" /d builtin /f
  wine reg add 'HKCU\Software\Wine\DllOverrides' /v "*wininet" /d builtin /f
  ```

### Launcher remains stuck on "Waiting for answer" after login
- **Cause**: The host browser cannot dispatch `wwise-launcher://` URLs.
- **Fix**: Register the desktop MIME handler:
  ```bash
  ./scripts/setup-desktop.sh
  ```
  Verify with:
  ```bash
  xdg-mime query default x-scheme-handler/wwise-launcher
  # Expected: wwise-launcher.desktop
  ```

### "Unable to start the installation process" during component install
- **Cause**: Detached `.lnk` race condition. The `cmdwait` shim is either missing or `ComSpec` is not set.
- **Fix**: Re-run `./scripts/setup-shims.sh` and verify:
  ```bash
  wine cmd /c "echo %ComSpec%"
  # Expected: C:\windows\cmdwait\cmd.exe
  ```

### Audio Device Issues in Wwise Authoring
- **PulseAudio / PipeWire**: Ensure `xaudio2_9` is set to native,builtin:
  ```bash
  winetricks -q xaudio29
  ```
- **ALSA / Jack**: If using Pro Audio (JACK/PipeWire-JACK), run `winecfg` -> **Audio** tab and confirm the output device is recognized.

---

## 3. Wine Logging & Debugging

To run with verbose logging:
```bash
WINEDEBUG=+loaddll,+process ./bin/wwise gui
```

To monitor the OAuth protocol handler:
```bash
tail -f url-handler.log
```
