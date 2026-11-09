/* Does urlmon.CreateUri resolve in this prefix?  That single GetProcAddress is
   what aborts the launcher the moment Sign In is clicked: urlmon forwards
   CreateUri to iertutil BY NAME, and native iertutil exports it by ordinal
   only (681 functions, 10 names).  Run under both overrides to see the flip. */
#include <windows.h>
#include <stdio.h>
typedef HRESULT (WINAPI *pCreateUri)(LPCWSTR, DWORD, DWORD_PTR, void **);
int main(void)
{
    HMODULE h = LoadLibraryA("urlmon.dll");
    pCreateUri f;
    void *uri = NULL;
    HRESULT hr;
    if (!h) { printf("RESULT: urlmon.dll would not load\n"); return 1; }
    f = (pCreateUri)GetProcAddress(h, "CreateUri");
    if (!f) { printf("RESULT: FAIL - CreateUri unresolved (err %lu)\n", GetLastError()); return 2; }
    hr = f(L"https://www.audiokinetic.com/login", 0, 0, &uri);
    printf("RESULT: OK - CreateUri=%p hr=0x%08lx uri=%p\n", (void *)f, (unsigned long)hr, uri);
    return 0;
}
