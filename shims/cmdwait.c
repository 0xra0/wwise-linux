/* cmdwait.exe - ComSpec wrapper that restores one thing Wine's cmd does not do:
 * waiting for a .lnk.
 *
 * The Wwise Launcher elevates by writing runAsAdmin.lnk (target = its own exe,
 * args = --installPrerequisites=<json> --stdoutFile=<f> --stderrFile=<f>),
 * flipping the RunAsAdmin bit, and then exec()ing the shortcut path. On Windows
 * cmd blocks until the elevated child exits. Under Wine, shell32 resolves the
 * link and launches the target DETACHED - measured at 0.29s against a target
 * that runs for 3s - so runSelfAsAdmin() reads stdout.txt while it is still the
 * empty file it just wrote, `if(!n) throw` fires, and the install dies with
 * "Unable to start the installation process". The prerequisites themselves
 * succeed 1.5s later, into a file whose parent already gave up on it.
 *
 * Neither the lnkfile shell association nor PATHEXT changes that: both were
 * tested, both still ran the target detached.
 *
 * So: node's child_process.exec() spawns process.env.comspec. Point ComSpec at
 * this, and for a command mentioning a .lnk we run cmd as usual and then wait
 * for %TEMP%\WwiseLauncher\stdout.txt to appear AND STOP GROWING before
 * returning. The parent then reads the real output the real child really wrote.
 * Everything else is handed to the real cmd.exe verbatim and waited on, which
 * is what cmd would have done anyway.
 *
 * Deliberately does NOT fabricate content: on a genuine prerequisite failure the
 * file stays empty, we time out, and the launcher reports the same error it
 * would have on Windows.
 *
 * D91 - IT MUST BE INSTALLED UNDER THE FILENAME cmd.exe.
 * node decides how to pass a command to the shell by looking at the shell's
 * NAME, not at what it is:
 *
 *     if (/^(?:.*\\)?cmd(?:\.exe)?$/i.test(file))
 *          args = ['/d','/s','/c', '"'+command+'"'];   // Windows style
 *     else args = ['-c', command];                     // POSIX style
 *
 * Installed as system32\cmdwait.exe it failed that test, so every exec() in the
 * launcher arrived as `-c net use C:`. cmd has no -c, ignores it, and opens an
 * INTERACTIVE session on a stdin nobody will ever write to - a second call that
 * never returns, which left the install page's Next button greyed out forever
 * because system:validateDirectory could not resolve. Live under CDP: exec via
 * ComSpec pending at 25s, the identical execFile straight to cmd.exe 330ms.
 *
 * So it lives at C:\windows\cmdwait\cmd.exe - a directory of its own, the name
 * node insists on - and REAL_CMD still points at the genuine system32\cmd.exe,
 * which is a different path, so there is no recursion. The -c translation below
 * is a belt-and-braces second line of defence, not the fix.
 *
 *   x86_64-w64-mingw32-gcc -O2 -o cmdwait.exe cmdwait.c
 */
#include <windows.h>
#include <stdio.h>

#define POLL_MS      100
#define STABLE_POLLS 4               /* ~400ms unchanged = the child is done */
#define TIMEOUT_MS   240000          /* then give up and let the app decide */

static const wchar_t *REAL_CMD = L"C:\\windows\\system32\\cmd.exe";

/* Skip argv[0] in a raw command line, quoted or not. */
static wchar_t *skip_argv0(wchar_t *p)
{
    if (*p == L'"') {
        p++;
        while (*p && *p != L'"') p++;
        if (*p == L'"') p++;
    } else {
        while (*p && *p != L' ' && *p != L'\t') p++;
    }
    while (*p == L' ' || *p == L'\t') p++;
    return p;
}

/* If we are still handed POSIX-style `-c <command>` (see D91), say it the way
 * cmd understands. cmd has no -c switch at all, so nothing legitimate is lost. */
static wchar_t *posix_to_cmd(wchar_t *p)
{
    static wchar_t fixed[32768];

    if (!(p[0] == L'-' && p[1] == L'c' && (p[2] == L' ' || p[2] == L'\t')))
        return p;
    p += 3;
    while (*p == L' ' || *p == L'\t') p++;
    _snwprintf(fixed, sizeof(fixed)/sizeof(fixed[0]), L"/d /s /c \"%s\"", p);
    return fixed;
}

static int mentions_lnk(const wchar_t *s)
{
    for (; *s; s++)
        if ((s[0] == L'.') &&
            (s[1] == L'l' || s[1] == L'L') &&
            (s[2] == L'n' || s[2] == L'N') &&
            (s[3] == L'k' || s[3] == L'K'))
            return 1;
    return 0;
}

static LONGLONG file_size(const wchar_t *path)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    LARGE_INTEGER li;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &fad)) return -1;
    li.HighPart = fad.nFileSizeHigh;
    li.LowPart  = fad.nFileSizeLow;
    return li.QuadPart;
}

/* Wait for the detached prerequisite child to finish writing its stdout. */
static void wait_for_prereq_output(void)
{
    wchar_t path[MAX_PATH];
    DWORD n = GetTempPathW(MAX_PATH, path);
    LONGLONG last = -1;
    int stable = 0, waited = 0;

    if (!n || n > MAX_PATH - 32) return;
    wcscat(path, L"WwiseLauncher\\stdout.txt");

    while (waited < TIMEOUT_MS) {
        LONGLONG sz = file_size(path);
        if (sz > 0) {
            stable = (sz == last) ? stable + 1 : 0;
            if (stable >= STABLE_POLLS) return;
        }
        last = sz;
        Sleep(POLL_MS);
        waited += POLL_MS;
    }
}

int main(void)
{
    wchar_t *args = posix_to_cmd(skip_argv0(GetCommandLineW()));
    int is_lnk = mentions_lnk(args);
    size_t len = wcslen(REAL_CMD) + wcslen(args) + 8;
    wchar_t *line = (wchar_t *)malloc(len * sizeof(wchar_t));
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD code = 1;

    if (!line) return 1;
    /* Rebuild the line cmd itself would have seen: only argv[0] changes. */
    _snwprintf(line, len, L"\"%s\" %s", REAL_CMD, args);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(NULL, line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fwprintf(stderr, L"cmdwait: cannot start %s (%lu)\n", REAL_CMD, GetLastError());
        return 1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (is_lnk) wait_for_prereq_output();
    return (int)code;
}
