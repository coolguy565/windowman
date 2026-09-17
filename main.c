#define _UNICODE

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WINDOWS 4096

static HICON g_hIcon;
static const wchar_t *g_iconPath;
static const wchar_t *g_title;

typedef struct {
    HWND windows[MAX_WINDOWS];
    int count;
} WindowList;

typedef struct {
    WindowList *before;
    const wchar_t *title;
    int applied;
} EnumCtx;

static void ApplyIcon(HWND hwnd) {
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    int cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    int cySmIcon = GetSystemMetrics(SM_CYSMICON);

    HICON hBig = (HICON)LoadImageW(NULL, g_iconPath, IMAGE_ICON, cxIcon, cyIcon, LR_LOADFROMFILE);
    HICON hSmall = (HICON)LoadImageW(NULL, g_iconPath, IMAGE_ICON, cxSmIcon, cySmIcon, LR_LOADFROMFILE);

    if (!hBig) hBig = CopyIcon(g_hIcon);
    if (!hSmall) hSmall = CopyIcon(g_hIcon);

    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hBig);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hSmall);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL2, (LPARAM)hSmall);

    SetClassLongPtrW(hwnd, GCLP_HICON, (LONG_PTR)hBig);
    SetClassLongPtrW(hwnd, GCLP_HICONSM, (LONG_PTR)hSmall);

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

static BOOL CALLBACK EnumBeforeProc(HWND hwnd, LPARAM lParam) {
    WindowList *list = (WindowList *)lParam;
    if (list->count < MAX_WINDOWS)
        list->windows[list->count++] = hwnd;
    return TRUE;
}

static BOOL IsWindowNew(HWND hwnd, WindowList *before) {
    for (int i = 0; i < before->count; i++) {
        if (before->windows[i] == hwnd)
            return FALSE;
    }
    return TRUE;
}

static BOOL CALLBACK EnumAfterProc(HWND hwnd, LPARAM lParam) {
    EnumCtx *ctx = (EnumCtx *)lParam;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    if (!IsWindowNew(hwnd, ctx->before))
        return TRUE;

    if (ctx->title) {
        wchar_t buf[256] = {0};
        GetWindowTextW(hwnd, buf, 256);
        if (wcsstr(buf, ctx->title) == NULL)
            return TRUE;
    }

    ApplyIcon(hwnd);
    ctx->applied++;
    return TRUE;
}

static BOOL CreateTempShortcut(const wchar_t *shortcutPath, const wchar_t *targetPath,
                                const wchar_t *args, const wchar_t *iconPath) {
    IShellLinkW *pShellLink = NULL;
    IPersistFile *pPersistFile = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IShellLinkW, (void **)&pShellLink);
    if (FAILED(hr)) return FALSE;

    pShellLink->lpVtbl->SetPath(pShellLink, targetPath);
    pShellLink->lpVtbl->SetArguments(pShellLink, args);
    pShellLink->lpVtbl->SetIconLocation(pShellLink, iconPath, 0);

    hr = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile, (void **)&pPersistFile);
    if (FAILED(hr)) {
        pShellLink->lpVtbl->Release(pShellLink);
        return FALSE;
    }

    hr = pPersistFile->lpVtbl->Save(pPersistFile, shortcutPath, TRUE);
    pPersistFile->lpVtbl->Release(pPersistFile);
    pShellLink->lpVtbl->Release(pShellLink);

    return SUCCEEDED(hr);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    const wchar_t *icon = NULL;
    const wchar_t *title = NULL;
    int keepalive = 0;
    int exe_idx = -1;

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"--") == 0) { exe_idx = i + 1; break; }
        if (wcscmp(argv[i], L"--icon") == 0 && i + 1 < argc) { icon = argv[++i]; }
        else if (wcscmp(argv[i], L"--title") == 0 && i + 1 < argc) { title = argv[++i]; }
        else if (wcscmp(argv[i], L"--keepalive") == 0) { keepalive = 1; }
    }

    if (!icon || exe_idx < 0 || exe_idx >= argc) {
        MessageBoxW(NULL, L"Usage: WindowMan.exe --icon <file.ico> -- <program> [args...]",
                    L"WindowMan", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_hIcon = (HICON)LoadImageW(NULL, icon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!g_hIcon) {
        MessageBoxW(NULL, L"Failed to load icon file.", L"WindowMan", MB_OK | MB_ICONERROR);
        return 1;
    }
    g_iconPath = icon;
    g_title = title;

    CoInitialize(NULL);

    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);

    wchar_t shortcutPath[MAX_PATH];
    wcscpy(shortcutPath, tempDir);
    wcscat(shortcutPath, L"WindowMan_temp.lnk");

    wchar_t args[32768] = {0};
    for (int i = exe_idx + 1; i < argc; i++) {
        if (i > exe_idx + 1) wcscat(args, L" ");
        wcscat(args, argv[i]);
    }

    if (!CreateTempShortcut(shortcutPath, argv[exe_idx], args, icon)) {
        CoUninitialize();
        return 1;
    }

    WindowList before = {0};
    EnumWindows(EnumBeforeProc, (LPARAM)&before);

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = shortcutPath;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {
        CoUninitialize();
        return 1;
    }

    DWORD start = GetTickCount();
    EnumCtx ctx = {0};
    ctx.before = &before;
    ctx.title = title;
    ctx.applied = 0;

    while (GetTickCount() - start < 30000) {
        DWORD status = WaitForSingleObject(sei.hProcess, 500);
        if (status == WAIT_OBJECT_0 && !keepalive)
            break;

        ctx.applied = 0;
        EnumWindows(EnumAfterProc, (LPARAM)&ctx);
        if (ctx.applied > 0)
            break;
    }

    if (keepalive)
        WaitForSingleObject(sei.hProcess, INFINITE);

    DestroyIcon(g_hIcon);
    CloseHandle(sei.hProcess);
    CoUninitialize();

    return 0;
}
