/* powershell.exe for a Wine prefix, sized to exactly one caller.
 *
 * The Audiokinetic Launcher is Electron, and its startup calls the npm package
 * `systeminformation`, whose processes() on Windows runs
 *
 *     powershell Get-CimInstance Win32_Process | select-Object ProcessId,
 *       ParentProcessId,ExecutionState,Caption,CommandLine,ExecutablePath,
 *       UserModeTime,KernelModeTime,WorkingSetSize,Priority,PageFileUsage,
 *       @{n="CreationDate";...} | fl
 *
 * Wine ships a powershell.exe that exits 0 having printed nothing, so the
 * parser gets an empty string, the launcher's own wrapper throws
 * "Failed to retrieve running processes" (i18n key
 * system:process.failedToRetrieveProcess) out of initCoreServices, and the app
 * quits before it ever draws a window.
 *
 * This answers that ONE query with a real process list from
 * CreateToolhelp32Snapshot, in PowerShell's format-list shape: "Key : Value"
 * lines, CRLF, one blank line between records. Every other command produces an
 * empty answer, exactly as the Wine stub does - systeminformation asks
 * powershell about the BIOS, the baseboard and the OS too, and the launcher
 * already survived all of those coming back empty.
 *
 * The query is recognised by "ParentProcessId", which no other systeminformation
 * powershell command contains. Matching "Win32_Process" would also swallow
 * Win32_Processor.
 *
 * The launcher's OWN check is a different command again, and it is the one that
 * decides whether the app starts. getRunningProcesses() runs
 *
 *     child_process.exec('echo ""; ps | % {$_.Path}', { shell: "powershell" })
 *
 * which Node turns into `powershell -c 'echo ""; ps | % {$_.Path}'`, splits the
 * output on newlines, DROPS THE FIRST LINE (that is what the echo "" is for),
 * and throws system:process.failedToRetrieveProcess if what is left is empty.
 * Wine's powershell exits 0 having printed nothing, so the list is empty and the
 * launcher quits - and because the exit code is 0, its own `catch` never fires
 * and it never reaches the wmic fallback that would have worked here.
 *
 * THE SYSTEMINFORMATION COMMANDS ARRIVE ON STDIN, NOT IN ARGV. systeminformation keeps ONE
 * powershell alive - `powershell.exe -NoProfile -NoLogo -InputFormat Text
 * -NoExit -Command -` - and pushes three lines per query into its stdin:
 *
 *     $OutputEncoding = ... ; echo --###START###--<id>--##ID##--;
 *     <the actual command>
 *     echo --###ENDCMD###--
 *
 * and reads stdout until it sees --###ENDCMD###--, splitting the id back out on
 * --##ID##--. So this has to be a REPL that honours those two echoes; a version
 * that only looked at argv (the first one written here) changed nothing,
 * because argv is just "-Command -".
 *
 * Install:  x86_64-w64-mingw32-gcc -O2 -o powershell.exe powershell_shim.c
 *           cp powershell.exe $WINEPREFIX/drive_c/windows/system32/
 * Remove:   delete that one file; the Wine builtin comes back.
 */
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

static void trace(const char *what, const char *text);
static int  contains(const char *h, const char *n);

static void put(const char *k, const char *v)
{
    printf("%-18s: %s\r\n", k, v ? v : "");
}

static void putn(const char *k, unsigned long long v)
{
    printf("%-18s: %llu\r\n", k, v);
}

/* FILETIME -> "yyyy-MM-dd HH:mm:ss" in local time, or "" if it is not set. */
static void creation_date(const FILETIME *ft, char *out, size_t n)
{
    SYSTEMTIME utc, loc;
    out[0] = 0;
    if (!ft->dwLowDateTime && !ft->dwHighDateTime) return;
    if (!FileTimeToSystemTime(ft, &utc)) return;
    if (!SystemTimeToTzSpecificLocalTime(NULL, &utc, &loc)) loc = utc;
    snprintf(out, n, "%04d-%02d-%02d %02d:%02d:%02d",
             loc.wYear, loc.wMonth, loc.wDay, loc.wHour, loc.wMinute, loc.wSecond);
}

static unsigned long long ft100ns(const FILETIME *ft)
{
    return ((unsigned long long)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
}

/* `ps | %% {$_.Path}` - one full image path per line, blank if unknown. */
static void emit_paths(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    if (snap == INVALID_HANDLE_VALUE) return;
    pe.dwSize = sizeof(pe);
    if (!Process32First(snap, &pe)) { CloseHandle(snap); return; }
    do {
        char path[MAX_PATH] = "";
        HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
        if (p) {
            DWORD len = MAX_PATH;
            if (QueryFullProcessImageNameA(p, 0, path, &len) == 0) path[0] = 0;
            CloseHandle(p);
        }
        /* No path is what PowerShell prints for a process it cannot open, and
           the caller filters empty lines out - so print the name instead of
           nothing, which is more useful and just as legal. */
        printf("%s\r\n", path[0] ? path : pe.szExeFile);
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
}

static void emit_processes(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    if (snap == INVALID_HANDLE_VALUE) return;

    pe.dwSize = sizeof(pe);
    if (!Process32First(snap, &pe)) { CloseHandle(snap); return; }

    do {
        char path[MAX_PATH] = "";
        char created[32] = "";
        unsigned long long user = 0, kern = 0, ws = 0;
        HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);

        if (p) {
            FILETIME c, e, k, u;
            DWORD len = MAX_PATH;
            if (QueryFullProcessImageNameA(p, 0, path, &len) == 0) path[0] = 0;
            if (GetProcessTimes(p, &c, &e, &k, &u)) {
                creation_date(&c, created, sizeof(created));
                /* Win32_Process reports these in 100ns units, as GetProcessTimes does. */
                user = ft100ns(&u);
                kern = ft100ns(&k);
            }
            CloseHandle(p);
        }

        putn("ProcessId",       pe.th32ProcessID);
        putn("ParentProcessId", pe.th32ParentProcessID);
        put ("ExecutionState",  "");
        put ("Caption",         pe.szExeFile);
        put ("CommandLine",     path);
        put ("ExecutablePath",  path);
        putn("UserModeTime",    user);
        putn("KernelModeTime",  kern);
        putn("WorkingSetSize",  ws);
        putn("Priority",        (unsigned long long)pe.pcPriClassBase);
        putn("PageFileUsage",   0);
        put ("CreationDate",    created);
        printf("\r\n");
    } while (Process32Next(snap, &pe));

    CloseHandle(snap);
}

static int contains(const char *h, const char *n) { return strstr(h, n) != NULL; }

/* Set WWISE_SHIM_LOG to a Windows path to see what the caller actually sends. */
static void trace(const char *what, const char *text)
{
    const char *path = getenv("WWISE_SHIM_LOG");
    FILE *f;
    if (!path || !*path) return;
    f = fopen(path, "ab");
    if (!f) return;
    fprintf(f, "[%s] %s\n", what, text ? text : "");
    fclose(f);
}

/* One stdin line of the systeminformation REPL protocol. */
static void handle_line(char *line)
{
    char *p;

    /* echo --###START###--<id>--##ID##-- : hand the id straight back. */
    if ((p = strstr(line, "--###START###--")) != NULL) {
        char *end = strstr(p, "--##ID##--");
        if (end) {
            end += strlen("--##ID##--");
            fwrite(p, 1, (size_t)(end - p), stdout);
            printf("\r\n");
        }
        fflush(stdout);
        return;
    }

    /* echo --###ENDCMD###-- : the reader is waiting for exactly this. */
    if (contains(line, "--###ENDCMD###--")) {
        printf("--###ENDCMD###--\r\n");
        fflush(stdout);
        return;
    }

    /* The one command worth answering. */
    if (contains(line, "ParentProcessId")) emit_processes();
    fflush(stdout);
}

int main(int argc, char **argv)
{
    static char line[65536];
    int i, repl = 0;

    _setmode(_fileno(stdout), _O_BINARY);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* "-Command -" means "read commands from stdin". Anything else is one shot. */
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "-") || !strcmp(argv[i], "-NoExit")) repl = 1;

    if (!repl) {
        for (i = 1; i < argc; i++) {
            trace("argv", argv[i]);
            if (contains(argv[i], "ParentProcessId")) { emit_processes(); break; }
            /* getRunningProcesses(). The leading blank line is the `echo ""`
               the caller wrote and then discards with .slice(1); without it the
               first real process would be the one thrown away. */
            if (contains(argv[i], "$_.Path")) { printf("\r\n"); emit_paths(); break; }
        }
        return 0;
    }

    while (fgets(line, sizeof(line), stdin)) {
        if (!strncmp(line, "exit", 4)) break;
        handle_line(line);
    }
    return 0;
}
