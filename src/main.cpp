// Omni - read-only Instance Explorer (Dex-style dark UI)
// Entry point only. See README / structure in the project folder.
#include "common.h"
#include "state.h"
#include "util.h"
#include "layout.h"
#include "live.h"
#include "window.h"

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int show) {
    // Per-Monitor DPI v2 (if the manifest is missing); fall back to system DPI
    typedef BOOL (WINAPI *SetCtxFn)(HANDLE);
    auto setCtx = (SetCtxFn)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext");
    if (!(setCtx && setCtx((HANDLE)-4))) SetProcessDPIAware();

    EnableDarkMenus();

    HDC hdc = GetDC(nullptr);
    g_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);

    INITCOMMONCONTROLSEX icc{sizeof(icc),
        ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    if (!CreateMainWindow(hi, show)) return 1;

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        if (m.message == WM_KEYDOWN) {
            if (m.wParam == VK_F5) { PostMessageW(g_main, WM_COMMAND, MAKEWPARAM(ID_REFRESH, BN_CLICKED), 0); continue; }
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (ctrl && m.wParam == 'F') {
                if (!g_showExplorer) { g_showExplorer = true; RelayoutNow(); }
                SetFocus(g_edit); SendMessageW(g_edit, EM_SETSEL, 0, -1);
                continue;
            }
            if (ctrl && m.wParam == 'L') { SetLive(!g_live); continue; }
        }
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return (int)m.wParam;
}
