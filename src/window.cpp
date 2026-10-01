#include "window.h"
#include "state.h"
#include "util.h"
#include "gfx.h"
#include "theme.h"
#include "memory.h"
#include "icons.h"
#include "layout.h"
#include "paint.h"
#include "tree.h"
#include "props.h"
#include "menus.h"
#include "connection.h"
#include "live.h"

// ============================================================
//                       DPI / fonts
// ============================================================
static void UpdateDpi(HWND hwnd) {
    typedef UINT (WINAPI *GetDpiFn)(HWND);
    auto fn = (GetDpiFn)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (fn) { UINT d = fn(hwnd); if (d) g_dpi = (int)d; }
}
static void ApplyFonts() {
    HFONT nb  = MakeFont(13, FW_NORMAL);
    HFONT nB  = MakeFont(13, FW_SEMIBOLD);
    HFONT nS  = MakeFont(11, FW_NORMAL);
    HFONT nBr = MakeFont(15, FW_BOLD);
    HFONT nI  = MakeFont(20, FW_NORMAL, L"Segoe UI Symbol");
    HFONT nM  = MakeFont(12, FW_NORMAL, L"Consolas");
    for (HWND h : {g_edit, g_propFilter, g_tree, g_props})
        SendMessageW(h, WM_SETFONT, (WPARAM)nb, TRUE);
    if (g_tip) SendMessageW(g_tip, WM_SETFONT, (WPARAM)nS, TRUE);
    for (HFONT* f : {&g_fontBody, &g_fontBold, &g_fontSmall, &g_fontBrand, &g_fontIcon, &g_fontMono})
        if (*f) DeleteObject(*f);
    g_fontBody = nb; g_fontBold = nB; g_fontSmall = nS; g_fontBrand = nBr; g_fontIcon = nI; g_fontMono = nM;

    TreeView_SetItemHeight(g_tree, DP(24));
    TreeView_SetIndent(g_tree, DP(16));
    RebuildIcons();
    HIMAGELIST ni = ImageList_Create(1, DP(26), ILC_COLOR32, 1, 1);   // forces taller ListView rows
    HIMAGELIST old = ListView_SetImageList(g_props, ni, LVSIL_SMALL);
    if (old) ImageList_Destroy(old);
    g_rowImg = ni;
    LPARAM mg = MAKELONG(0, 0);
    SendMessageW(g_edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, mg);
    SendMessageW(g_propFilter, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, mg);
}

// ============================================================
//                       subclass procs
// ============================================================
static LRESULT CALLBACK EditProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR) {
    if (m == WM_CHAR && w == VK_RETURN) {
        PostMessageW(GetParent(h), WM_COMMAND, MAKEWPARAM(id == 1 ? ID_EDIT : ID_PROPFILTER, 0), 0);
        return 0;
    }
    if (m == WM_KEYDOWN && w == VK_ESCAPE) { SetWindowTextW(h, L""); return 0; }
    if (m == WM_KEYDOWN && w == VK_DOWN) { SetFocus(id == 1 ? g_tree : g_props); return 0; }
    if (m == WM_CHAR && w == VK_ESCAPE) return 0;
    return DefSubclassProc(h, m, w, l);
}
// Row hover on the Properties ListView
static LRESULT CALLBACK PropsProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    static bool tracking = false, busy = false;
    switch (m) {
    case WM_MOUSEMOVE: {
        LVHITTESTINFO hti{};
        hti.pt = POINT{(short)LOWORD(l), (short)HIWORD(l)};
        int row = ListView_HitTest(h, &hti);
        if (row != g_propHot) {
            int oldRow = g_propHot;
            g_propHot = row;
            int cnt = ListView_GetItemCount(h);
            if (oldRow >= 0 && oldRow < cnt) ListView_RedrawItems(h, oldRow, oldRow);
            if (row >= 0 && row < cnt) ListView_RedrawItems(h, row, row);
        }
        if (!tracking) {
            TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, h, 0};
            if (TrackMouseEvent(&t)) tracking = true;
        }
        break; }
    case WM_MOUSELEAVE: {
        tracking = false;
        if (g_propHot >= 0) {
            int oldRow = g_propHot; g_propHot = -1;
            if (oldRow < ListView_GetItemCount(h)) ListView_RedrawItems(h, oldRow, oldRow);
        }
        break; }
    case WM_SIZE: {
        LRESULT r = DefSubclassProc(h, m, w, l);
        if (!busy) { busy = true; ResizePropColumns(); busy = false; }
        return r; }
    case WM_NCDESTROY:
        RemoveWindowSubclass(h, PropsProc, 3);
        break;
    }
    return DefSubclassProc(h, m, w, l);
}

// ============================================================
//                       hit testing
// ============================================================
static bool EditHasText(HWND e) { return e && GetWindowTextLengthW(e) > 0; }
static int HdrHit(POINT p) {
    for (int i = 0; i < HB_COUNT; ++i)
        if (g_hb[i].right > g_hb[i].left && PtInRect(&g_hb[i], p)) return i;
    return -1;
}
static int ClearHit(POINT p) {
    if (g_showExplorer && g_ex.all.right > g_ex.all.left && EditHasText(g_edit)) {
        RECT r = ClearRect(g_ex); if (PtInRect(&r, p)) return 0;
    }
    if (g_showProps && g_pr.all.right > g_pr.all.left && EditHasText(g_propFilter)) {
        RECT r = ClearRect(g_pr); if (PtInRect(&r, p)) return 1;
    }
    return -1;
}
static bool ConnectHit(POINT p) {
    return g_rcConnect.right > g_rcConnect.left && PtInRect(&g_rcConnect, p);
}
static int SideHit(POINT p) {
    for (int i = 0; i < SIDE_ITEMS; ++i) if (PtInRect(&g_rcSideItem[i], p)) return i;
    return -1;
}
static bool SplitHit(POINT p) {
    if (g_rcSplit.right <= g_rcSplit.left) return false;
    RECT sh = g_rcSplit; InflateRect(&sh, DP(2), DP(2));
    return PtInRect(&sh, p) != FALSE;
}
static bool OnMainClick(HWND hwnd, POINT p, bool dbl) {
    int hb = HdrHit(p);
    if (hb >= 0) {
        if (hb == HB_EX_COLLAPSE)      CollapseAllTree();
        else if (hb == HB_EX_REFRESH)  DoRefresh();
        else if (hb == HB_PR_COLLAPSE) ToggleAllGroups();
        else if (hb == HB_PR_LIVE)     SetLive(!g_live);
        return true;
    }
    int cl = ClearHit(p);
    if (cl >= 0) {
        HWND e = (cl == 0) ? g_edit : g_propFilter;
        SetWindowTextW(e, L"");
        SetFocus(e);
        return true;
    }
    if (ConnectHit(p)) { DoRefresh(); return true; }
    int hit = SideHit(p);
    if (hit == 0) { if (!(g_showExplorer && !g_showProps)) g_showExplorer = !g_showExplorer; RelayoutNow(); return true; }
    if (hit == 1) { if (!(g_showProps && !g_showExplorer)) g_showProps = !g_showProps; RelayoutNow(); return true; }
    if (hit == 2) { DoRefresh(); return true; }
    if (hit == 3) { SetLive(!g_live); return true; }
    if (SplitHit(p)) {
        if (dbl) { g_split = 0.55; RelayoutNow(); }
        else { g_dragSplit = true; SetCapture(hwnd); InvalidateRect(hwnd, &g_rcSplit, FALSE); }
        return true;
    }
    return false;
}

// ============================================================
//                       window procedure
// ============================================================
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_main = hwnd;
        UpdateDpi(hwnd);
        g_brField = CreateSolidBrush(Theme::FIELD);

        g_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 0, 0, 10, 10, hwnd, (HMENU)ID_EDIT, nullptr, nullptr);
        g_tree = CreateWindowExW(0, WC_TREEVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_FULLROWSELECT |
                                 TVS_SHOWSELALWAYS | TVS_NOHSCROLL | TVS_TRACKSELECT,
                                 0, 0, 10, 10, hwnd, (HMENU)ID_TREE, nullptr, nullptr);
        TreeView_SetExtendedStyle(g_tree, TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS,
                                          TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS);
        TreeView_SetBkColor(g_tree, Theme::BG_PANEL);
        TreeView_SetTextColor(g_tree, Theme::TEXT);
        SetWindowTheme(g_tree, L"DarkMode_Explorer", nullptr);

        g_propFilter = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 0, 0, 10, 10, hwnd, (HMENU)ID_PROPFILTER, nullptr, nullptr);
        g_props = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
                                 LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
                                 0, 0, 10, 10, hwnd, (HMENU)ID_PROPS, nullptr, nullptr);
        ListView_SetExtendedListViewStyle(g_props,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        ListView_SetBkColor(g_props, Theme::BG_PANEL);
        ListView_SetTextBkColor(g_props, Theme::BG_PANEL);
        ListView_SetTextColor(g_props, Theme::TEXT);
        SetWindowTheme(g_props, L"DarkMode_Explorer", nullptr);

        LVCOLUMNW c{}; c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT; c.fmt = LVCFMT_LEFT;
        c.pszText = (LPWSTR)L"Property"; c.cx = DP(200); ListView_InsertColumn(g_props, 0, &c);
        c.pszText = (LPWSTR)L"Value";    c.cx = DP(260); ListView_InsertColumn(g_props, 1, &c);
        c.pszText = (LPWSTR)L"Offset";   c.cx = DP(90);  ListView_InsertColumn(g_props, 2, &c);

        g_tip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (g_tip) {
            SetWindowTheme(g_tip, L"", L"");
            SendMessageW(g_tip, TTM_SETTIPBKCOLOR, (WPARAM)Theme::BG_HDR, 0);
            SendMessageW(g_tip, TTM_SETTIPTEXTCOLOR, (WPARAM)Theme::TEXT, 0);
            SendMessageW(g_tip, TTM_SETDELAYTIME, TTDT_INITIAL, 350);
        }

        SendMessageW(g_edit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Filter game\u2026  (Ctrl+F)");
        SendMessageW(g_propFilter, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search Properties");
        SetWindowSubclass(g_edit, EditProc, 1, 0);
        SetWindowSubclass(g_propFilter, EditProc, 2, 0);
        SetWindowSubclass(g_props, PropsProc, 3, 0);

        ApplyFonts();

        // dark title bar (Win10 1809+ / Win11)
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
        COLORREF cap = Theme::BG_SIDE, txt = Theme::TEXT, bdr = Theme::BORDER;
        DwmSetWindowAttribute(hwnd, 35, &cap, sizeof(cap));
        DwmSetWindowAttribute(hwnd, 36, &txt, sizeof(txt));
        DwmSetWindowAttribute(hwnd, 34, &bdr, sizeof(bdr));
        DWORD pref = 2; DwmSetWindowAttribute(hwnd, 33, &pref, sizeof(pref));

        HICON big = LoadAppIcon(32), small = LoadAppIcon(16);
        if (big)   SendMessageW(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)big);
        if (small) SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small);

        SetTimer(hwnd, TIMER_ANIM, 60, nullptr);
        SetTimer(hwnd, TIMER_POLL, 2000, nullptr);
        if (g_live) SetTimer(hwnd, TIMER_LIVE, (UINT)g_liveInterval, nullptr);   // Live is on by default
        PostMessageW(hwnd, WM_APP, 0, 0);
        return 0;
    }
    case WM_APP: DoRefresh(); return 0;
    case WM_TIMER:
        if (wp == TIMER_DEBOUNCE) { KillTimer(hwnd, TIMER_DEBOUNCE); ApplySearch(); }
        else if (wp == TIMER_ANIM) {
            if (g_connected) {
                RECT d = g_rcDot; InflateRect(&d, DP(7), DP(7));
                InvalidateRect(hwnd, &d, FALSE);
            }
            if (g_flashUntil) {
                InvalidateRect(g_props, nullptr, FALSE);
                if (GetTickCount() > g_flashUntil + 100) g_flashUntil = 0;
            }
        }
        else if (wp == TIMER_LIVE) LiveTick();
        else if (wp == TIMER_POLL) PollConnection();
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: PaintMain(hwnd); return 0;
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) { Layout(LOWORD(lp), HIWORD(lp)); InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lp;
        mmi->ptMinTrackSize.x = DP(520);
        mmi->ptMinTrackSize.y = DP(440);
        return 0;
    }
    case WM_DPICHANGED: {
        g_dpi = HIWORD(wp);
        ApplyFonts();
        RECT* rc = (RECT*)lp;
        SetWindowPos(hwnd, nullptr, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        RelayoutNow();
        return 0;
    }

    // --- mouse: sidebar, header buttons, clear, splitter ---
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
        OnMainClick(hwnd, p, msg == WM_LBUTTONDBLCLK);
        return 0;
    }
    case WM_RBUTTONUP: {
        POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
        if (SideHit(p) == 3 || HdrHit(p) == HB_PR_LIVE) {
            POINT sp; GetCursorPos(&sp);
            ShowLiveMenu(sp);
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
        if (g_dragSplit) {
            int sp = DP(SPLIT_W);
            double f;
            if (!g_stacked) f = (double)(p.x - g_rcArea.left - sp / 2) / (double)((g_rcArea.right - g_rcArea.left) - sp);
            else            f = (double)(p.y - sp / 2) / (double)((g_rcArea.bottom - g_rcArea.top) - sp);
            g_split = (std::max)(0.15, (std::min)(0.85, f));
            RelayoutNow();
            return 0;
        }
        int sh = SideHit(p), hh = HdrHit(p), ch = ClearHit(p);
        bool hc = ConnectHit(p);
        if (sh != g_sideHot || hh != g_hotHb || ch != g_hotClear || hc != g_hotConnect) {
            g_sideHot = sh; g_hotHb = hh; g_hotClear = ch; g_hotConnect = hc;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (!g_trackLeave) {
            TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, hwnd, 0};
            if (TrackMouseEvent(&t)) g_trackLeave = true;
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        g_trackLeave = false;
        g_sideHot = -1; g_hotHb = -1; g_hotClear = -1; g_hotConnect = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP:
        if (g_dragSplit) { g_dragSplit = false; ReleaseCapture(); InvalidateRect(hwnd, &g_rcSplit, FALSE); }
        return 0;
    case WM_CAPTURECHANGED:
        if (g_dragSplit) { g_dragSplit = false; InvalidateRect(hwnd, &g_rcSplit, FALSE); }
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
            if (g_dragSplit || SplitHit(p)) {
                SetCursor(LoadCursor(nullptr, g_stacked ? IDC_SIZENS : IDC_SIZEWE));
                return TRUE;
            }
            if (SideHit(p) >= 0 || HdrHit(p) >= 0 || ClearHit(p) >= 0 || ConnectHit(p)) {
                SetCursor(LoadCursor(nullptr, IDC_HAND)); return TRUE;
            }
        }
        break;

    // --- edit colours ---
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, Theme::TEXT);
        SetBkColor(hdc, Theme::FIELD);
        return (LRESULT)g_brField;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == ID_REFRESH && code == BN_CLICKED) {
            DoRefresh();
        } else if (id == ID_EDIT) {
            if (code == EN_CHANGE) {
                KillTimer(hwnd, TIMER_DEBOUNCE); SetTimer(hwnd, TIMER_DEBOUNCE, 250, nullptr);
                InvalidateRect(hwnd, &g_ex.flt, FALSE);
            }
            else if (code == 0) { KillTimer(hwnd, TIMER_DEBOUNCE); ApplySearch(); }     // Enter
            else if (code == EN_SETFOCUS || code == EN_KILLFOCUS) {
                InvalidateRect(hwnd, &g_ex.flt, FALSE); InvalidateRect(hwnd, &g_ex.hdr, FALSE);
            }
        } else if (id == ID_PROPFILTER) {
            if (code == EN_CHANGE) { FilterProps(); InvalidateRect(hwnd, &g_pr.flt, FALSE); }
            else if (code == EN_SETFOCUS || code == EN_KILLFOCUS) {
                InvalidateRect(hwnd, &g_pr.flt, FALSE); InvalidateRect(hwnd, &g_pr.hdr, FALSE);
            }
        }
        return 0;
    }
    case WM_NOTIFY: {
        LPNMHDR h = (LPNMHDR)lp;
        if (h->hwndFrom == g_tree) {
            if (h->code == NM_CUSTOMDRAW) {
                return HandleTreeCustomDraw((LPNMTVCUSTOMDRAW)lp);
            } else if (h->code == TVN_ITEMEXPANDINGW) {
                auto* p = (NMTREEVIEWW*)lp;
                HTREEITEM it = p->itemNew.hItem;
                if (p->action == TVE_EXPAND) {
                    if (!g_populated.count(it)) {
                        g_populated.insert(it);
                        PopulateChildren(it, ItemAddr(it));
                    } else if (!g_filterActive) {
                        SyncNode(it);        // re-opened branch: bring it up to date
                    }
                }
            } else if (h->code == TVN_SELCHANGEDW) {
                HTREEITEM sel = ((NMTREEVIEWW*)lp)->itemNew.hItem;
                ShowProps(sel);
                if (sel) {
                    uintptr_t a = ItemAddr(sel);
                    SetStatus(W(InstName(a)) + L"  \u00B7  " + W(ClassName(a)) + L"  \u00B7  " + Hex(a));
                }
            } else if (h->code == NM_RCLICK) {
                POINT sp; GetCursorPos(&sp);
                POINT cp = sp; ScreenToClient(g_tree, &cp);
                TVHITTESTINFO hti{}; hti.pt = cp;
                HTREEITEM hit = TreeView_HitTest(g_tree, &hti);
                if (hit) { TreeView_SelectItem(g_tree, hit); ShowTreeMenu(hit, sp); }
                return 1;
            } else if (h->code == NM_SETFOCUS || h->code == NM_KILLFOCUS) {
                InvalidateRect(hwnd, &g_ex.hdr, FALSE);
            }
        } else if (h->hwndFrom == g_props) {
            if (h->code == NM_CUSTOMDRAW) return HandlePropsCustomDraw((LPNMLVCUSTOMDRAW)lp);
            else if (h->code == NM_CLICK) {
                auto* ia = (LPNMITEMACTIVATE)lp;
                LVHITTESTINFO hti{}; hti.pt = ia->ptAction;
                int row = ListView_HitTest(g_props, &hti);
                if (row >= 0 && row < (int)g_propRows.size() && g_propRows[row].kind == PK_Group)
                    ToggleGroup(g_propRows[row].name);
            }
            else if (h->code == NM_DBLCLK) {
                auto* ia = (LPNMITEMACTIVATE)lp;
                if (ia->iItem >= 0 && ia->iItem < (int)g_propRows.size() &&
                    g_propRows[ia->iItem].kind != PK_Group) {
                    CopyText(g_propRows[ia->iItem].text);
                    Toast(g_propRows[ia->iItem].text);
                }
            } else if (h->code == NM_RCLICK) {
                POINT sp; GetCursorPos(&sp);
                POINT cp = sp; ScreenToClient(g_props, &cp);
                LVHITTESTINFO hti{}; hti.pt = cp;
                int row = ListView_HitTest(g_props, &hti);
                if (row >= 0 && row < (int)g_propRows.size() && g_propRows[row].kind != PK_Group)
                    ShowPropMenu(row, sp);
                return 1;
            } else if (h->code == NM_SETFOCUS || h->code == NM_KILLFOCUS) {
                InvalidateRect(hwnd, &g_pr.hdr, FALSE);
            }
        }
        return 0;
    }
    case WM_DESTROY:
        if (g_proc) CloseHandle(g_proc);
        for (HFONT* f : {&g_fontBody, &g_fontBold, &g_fontSmall, &g_fontBrand, &g_fontIcon, &g_fontMono})
            if (*f) DeleteObject(*f);
        if (g_brField) DeleteObject(g_brField);
        if (g_icons) ImageList_Destroy(g_icons);
        if (g_rowImg) ImageList_Destroy(g_rowImg);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// Dark popup menus (Win10 1903+ / Win11), via uxtheme ordinals. Safe if unavailable.
void EnableDarkMenus() {
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (!ux) return;
    typedef int  (WINAPI *SetPreferredAppModeFn)(int);
    typedef void (WINAPI *FlushMenuThemesFn)();
    auto setMode = (SetPreferredAppModeFn)(void*)GetProcAddress(ux, MAKEINTRESOURCEA(135));
    auto flush   = (FlushMenuThemesFn)(void*)GetProcAddress(ux, MAKEINTRESOURCEA(136));
    if (setMode) setMode(2);   // ForceDark
    if (flush) flush();
}

bool CreateMainWindow(HINSTANCE hi, int show) {
    WNDCLASSW wc{};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;                       // painted in WM_PAINT
    wc.lpszClassName = L"OmniWnd";
    if (!RegisterClassW(&wc)) return false;

    RECT wa; SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int W0 = (std::min)(DP(1180), (int)(wa.right - wa.left) * 9 / 10);
    int H0 = (std::min)(DP(780),  (int)(wa.bottom - wa.top) * 9 / 10);

    HWND w = CreateWindowExW(0, wc.lpszClassName, L"Omni",
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             CW_USEDEFAULT, CW_USEDEFAULT, W0, H0, nullptr, nullptr, hi, nullptr);
    if (!w) return false;
    ShowWindow(w, show);
    UpdateWindow(w);
    return true;
}
