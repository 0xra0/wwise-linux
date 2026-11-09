# Wwise on Linux Architecture & Compatibility Internals

Running Audiokinetic Wwise on Linux through Wine historically fails at several distinct stages due to subtle interactions between Wine's Windows emulation layer, Electron, and Audiokinetic's proprietary installer stack. This document details each problem, the measurements taken to isolate it, and the engineered solutions implemented in this repository.

---

## 1. The `urlmon.dll` / `iertutil.dll` Forwarding Collision

### The Symptom
When installing prerequisites (e.g. `vcrun2022`) or clicking **Sign In** within the Wwise Launcher, the application or thread crashes immediately without drawing a browser window or launching the authentication flow.

### Root Cause Analysis (Measured via `winedump`)
Wine's builtin `urlmon.dll` forwards `CreateUri` **by name** to `iertutil.CreateUri`.
However, Microsoft's Windows 7 `iertutil.dll` (which is pulled in whenever `wininet` is installed natively via winetricks) **exports functions by ordinal only**—it has zero exported function names.

When `iertutil=native` is set at the Wine prefix level:
1. Wine tries to resolve the forward `iertutil.CreateUri` by symbol name.
2. The native DLL lacks exported symbol names, causing the forward to fail.
3. Wine inserts an unimplemented-stub thunk.
4. Any caller of `urlmon.CreateUri` aborts immediately with error 127 (`ERROR_PROC_NOT_FOUND`).
5. `vc_redist.x86.exe` and Electron's `shell.openExternal()` both depend on this call.

Conversely:
- Native `wininet.dll` imports `iertutil` ordinal 650.
- Wine's builtin `iertutil` only exports ordinals up to 9.
- If native `wininet` is loaded with builtin `iertutil`, `wineboot` crashes at `initialize_internet`.

### The Solution
Keep the Wine prefix globally configured with:
```reg
[Software\Wine\DllOverrides]
"*iertutil"="builtin"
"*wininet"="builtin"
```
The installer runner (`bin/wwise install-launcher`) selectively overrides `WINEDLLOVERRIDES="wininet,iertutil=n,b"` scoped **only to the installer process**. The installed application uses builtin DLLs where `CreateUri` is cleanly resolved.

A verification tool is included at `shims/uritest.c` to test this resolution dynamically.

---

## 2. Electron Process Tree Check & The PowerShell Shim

### The Symptom
The Wwise Launcher immediately exits upon startup with:
```
Failed to retrieve running processes (system:process.failedToRetrieveProcess)
```
No window is drawn.

### Root Cause Analysis
The Wwise Launcher is an Electron application. During its `initCoreServices` startup phase, it invokes the npm package `systeminformation`. On Windows, `systeminformation` queries running processes using PowerShell:
```powershell
Get-CimInstance Win32_Process | select-Object ProcessId,ParentProcessId,Caption,CommandLine,ExecutablePath... | fl
```
Additionally, the Launcher runs its own process probe:
```js
child_process.exec('echo ""; ps | % {$_.Path}', { shell: "powershell" })
```
Wine includes a stub `powershell.exe` that simply exits with code 0 without producing any output. When Node parses empty output, it throws an exception and halts.

Crucially, `systeminformation` does not pass commands through `argv`; it keeps a single PowerShell session alive:
```
powershell.exe -NoProfile -NoLogo -InputFormat Text -NoExit -Command -
```
It streams commands over **STDIN** using a custom framing protocol:
```
$OutputEncoding = ... ; echo --###START###--<id>--##ID##--;
<the actual command>
echo --###ENDCMD###--
```

### The Solution: `powershell_shim.c`
We wrote a dedicated lightweight C replacement compiled to native PE64 `powershell.exe` placed at `%SystemRoot%\system32\powershell.exe`.

- It implements the STDIN REPL loop and responds to the `--###START###--` and `--###ENDCMD###--` delimiters.
- It intercepts queries matching `ParentProcessId` and queries the real Wine process tree using Win32 `CreateToolhelp32Snapshot` and `QueryFullProcessImageNameA`.
- It formats output into PowerShell's standard `Key : Value\r\n` list shape.
- When invoked in one-shot mode (`-c 'echo ""; ps | % {$_.Path}'`), it prints a newline followed by process paths.
- It is enabled via `WINEDLLOVERRIDES="powershell.exe=n"`.

---

## 3. Detached `.lnk` Elevation Race Condition (`cmdwait`)

### The Symptom
When installing Wwise versions, components, or prerequisites from the Launcher, the installation fails within half a second with:
```
Unable to start the installation process
```

### Root Cause Analysis
To perform elevated actions without prompting UAC repeatedly, Wwise Launcher creates a Windows shortcut:
`%TEMP%\WwiseLauncher\runAsAdmin.lnk` -> `Wwise Launcher.exe --installPrerequisites=... --stdoutFile=%TEMP%\WwiseLauncher\stdout.txt`
It then spawns this shortcut via `cmd.exe`.

On native Windows, `cmd.exe` waits for the shortcut process to terminate.
Under Wine, `shell32` resolves the `.lnk` and executes the target as a **detached asynchronous process** (returning in ~0.29s while the child runs for 3+ seconds).
The Launcher reads `stdout.txt` immediately, finds an empty file, triggers `if (!n) throw`, and aborts.

Furthermore, Node's `child_process.exec` checks the filename of the shell (`ComSpec`):
```js
if (/^(?:.*\\)?cmd(?:\.exe)?$/i.test(file))
    args = ['/d', '/s', '/c', '"' + command + '"']; // Windows style
else
    args = ['-c', command];                          // POSIX style
```
If `ComSpec` is pointed to a binary named anything other than `cmd.exe`, Node passes `-c net use C:`. Wine's `cmd` has no `-c` switch, opens an interactive prompt, and hangs indefinitely.

### The Solution: `cmdwait.c`
1. Compiled as `cmd.exe` located at `C:\windows\cmdwait\cmd.exe` (satisfying Node's regex check).
2. Registered in Wine registry as the system shell:
   `HKLM\System\CurrentControlSet\Control\Session Manager\Environment\ComSpec = %SystemRoot%\cmdwait\cmd.exe`
3. When executing a command containing `.lnk`, it spawns the real `C:\windows\system32\cmd.exe` and polls `%TEMP%\WwiseLauncher\stdout.txt` until file size stabilizes before returning exit status.

---

## 4. Host-to-Wine OAuth URL Redirection

### The Symptom
Clicking **Sign In** opens the Audiokinetic login page in Firefox/Chrome on Linux. After completing authentication, the browser attempts to redirect to `wwise-launcher://callback?code=...`, but nothing happens and the Launcher remains stuck at "Waiting for answer".

### Root Cause Analysis
The custom URL scheme `wwise-launcher://` is registered only in the Wine prefix registry. Native Linux web browsers have no awareness of Wine registry keys and cannot invoke Wine applications directly.

### The Solution
We register a standard FreeDesktop MIME handler on the Linux host:
- File: `~/.local/share/applications/wwise-launcher.desktop`
- Associated with:
  - `x-scheme-handler/wwise-launcher`
  - `x-scheme-handler/ak-launcher`
  - `x-scheme-handler/ak-bug-reporter`
- When invoked by the host browser, it passes the URL to `wwise-url-handler` (`bin/wwise url "$1"`).
- Electron's single-instance lock detects the argument and forwards it to the running Launcher instance via its second-instance handler, completing authentication instantly.
