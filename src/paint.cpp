#include "paint.h"
#include "state.h"
#include "util.h"
#include "gfx.h"
#include "theme.h"
#include "props.h"
#include "live.h"

// ============================================================
//                       status bar text
// ============================================================
void SetStatus(const std::wstring& s) {
    g_statusText = s;
    if (g_main) InvalidateRect(g_main, &g_rcStatus, FALSE);
}
void Toast(const std::wstring& what) {
    std::wstring s = what;
    if (s.size() > 80) s = s.substr(0, 80) + L"\u2026";
    SetStatus(L"Copied to clipboard  \u2014  " + s);
}

// ============================================================
//                       small widgets
// ============================================================
static void DrawSideIcon(HDC dc, int kind, int cx, int cy, COLORREF c) {
    auto R = [&](int x1, int y1, int x2, int y2) {
        RECT r = {cx + DP(x1), cy + DP(y1), cx + DP(x2), cy + DP(y2)};
        FillC(dc, r, c);
    };
    if (kind == 0) {            // Explorer: tree
        R(-9, -9, 9, -6);
        R(-7, -6, -5, 7);
        R(-5, -1, 9, 2);
        R(-5, 5, 9, 8);
    } else if (kind == 1) {     // Properties: list
        for (int i = 0; i < 3; ++i) {
            int y = -9 + i * 7;
            R(-9, y, -5, y + 4);
            R(-2, y + 1, 9, y + 3);
        }
    } else if (kind == 2) {     // Refresh
        HFONT of = (HFONT)SelectObject(dc, g_fontIcon);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, c);
        RECT t = {cx - DP(14), cy - DP(14), cx + DP(14), cy + DP(14)};
        DrawTextW(dc, L"\u21BB", -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, of);
    } else {                    // Live: dot with a ring
        PIC_FillCircle(dc, cx, cy, DP(3), c, c);
        DrawRing(dc, cx, cy, DP(8), DP(2), c);
    }
}
static void DrawMagnifier(HDC dc, int cx, int cy, COLORREF c) {
    int r = DP(4);
    HPEN pen = CreatePen(PS_SOLID, (std::max)(1, DP(1)), c);
    HGDIOBJ op = SelectObject(dc, pen);
    HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
    MoveToEx(dc, cx + r - DP(1), cy + r - DP(1), nullptr);
    LineTo(dc, cx + r + DP(3), cy + r + DP(3));
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
}
RECT ClearRect(const PaneRects& p) {
    return RECT{p.flt.right - DP(28), p.flt.top, p.flt.right - DP(4), p.flt.bottom};
}
static void DrawClear(HDC dc, RECT r, bool hot) {
    int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2, rad = DP(7);
    COLORREF bg = hot ? Theme::BORDER : Theme::GROUP_HDR_BG;
    PIC_FillCircle(dc, cx, cy, rad, bg, bg);
    HPEN pen = CreatePen(PS_SOLID, (std::max)(1, DP(1)), hot ? Theme::TEXT : Theme::TEXT_MUTED);
    HGDIOBJ op = SelectObject(dc, pen);
    int d = DP(3);
    MoveToEx(dc, cx - d, cy - d, nullptr); LineTo(dc, cx + d + 1, cy + d + 1);
    MoveToEx(dc, cx + d, cy - d, nullptr); LineTo(dc, cx - d - 1, cy + d + 1);
    SelectObject(dc, op); DeleteObject(pen);
}
static void DrawHdrBtn(HDC dc, int id) {
    RECT r = g_hb[id];
    if (r.right <= r.left) return;
    bool hot = (g_hotHb == id);
    bool on = (id == HB_PR_LIVE && g_live);
    if (hot || on) {
        COLORREF bg = on ? Mix(Theme::BG_HDR, Theme::ACCENT, 0.25) : Theme::HOVER;
        PIC_FillRounded(dc, r, DP(8), bg, bg);
    }
    COLORREF c = on ? Theme::ACCENT : (hot ? Theme::TEXT : Theme::TEXT_MUTED);
    int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
    if (id == HB_EX_COLLAPSE || id == HB_PR_COLLAPSE) {
        bool plus = (id == HB_PR_COLLAPSE) && !AnyGroupOpen();
        int s = DP(5);
        HPEN pen = CreatePen(PS_SOLID, (std::max)(1, DP(1)), c);
        HGDIOBJ op = SelectObject(dc, pen);
        HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, cx - s, cy - s, cx + s + 1, cy + s + 1, DP(3), DP(3));
        MoveToEx(dc, cx - s + DP(2), cy, nullptr); LineTo(dc, cx + s - DP(2) + 1, cy);
        if (plus) { MoveToEx(dc, cx, cy - s + DP(2), nullptr); LineTo(dc, cx, cy + s - DP(2) + 1); }
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
    } else if (id == HB_EX_REFRESH) {
        HFONT of = (HFONT)SelectObject(dc, g_fontIcon);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, c);
        RECT t = r;
        DrawTextW(dc, L"\u21BB", -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, of);
    } else {
        PIC_FillCircle(dc, cx, cy, DP(3), c, c);
        DrawRing(dc, cx, cy, DP(6), DP(1), c);
    }
}

// ============================================================
//                       panes
// ============================================================
static void PaintPane(HDC m, const PaneRects& p, const wchar_t* title, HWND edit,
                      bool focused, const std::wstring& sub, int btnLeftId) {
    if (p.all.right <= p.all.left) return;
    FillC(m, p.all, Theme::BG_PANEL);
    FillC(m, p.hdr, Theme::BG_HDR);
    HLine(m, p.hdr.left, p.hdr.right, p.hdr.bottom - 1, Theme::BORDER);
    if (focused) { RECT ab = {p.hdr.left, p.hdr.bottom - DP(2), p.hdr.right, p.hdr.bottom}; FillC(m, ab, Theme::ACCENT); }
    HFONT of = (HFONT)SelectObject(m, g_fontBold);
    SetBkMode(m, TRANSPARENT);
    SetTextColor(m, Theme::TEXT);
    RECT t = {p.hdr.left + DP(12), p.hdr.top, p.hdr.right - DP(8), p.hdr.bottom};
    DrawTextW(m, title, -1, &t, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SIZE ts{}; GetTextExtentPoint32W(m, title, (int)wcslen(title), &ts);
    if (!sub.empty()) {
        int btnLeft = g_hb[btnLeftId].right > g_hb[btnLeftId].left ? g_hb[btnLeftId].left : p.hdr.right;
        RECT st = {p.hdr.left + DP(12) + ts.cx + DP(10), p.hdr.top, btnLeft - DP(8), p.hdr.bottom - DP(1)};
        if (st.right - st.left > DP(30)) {
            SelectObject(m, g_fontSmall);
            SetTextColor(m, Theme::TEXT_FAINT);
            DrawTextW(m, sub.c_str(), -1, &st, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
    }
    SelectObject(m, of);

    bool fieldFocus = (GetFocus() == edit);
    PIC_FillRounded(m, p.flt, DP(14), Theme::FIELD, fieldFocus ? Theme::ACCENT : Theme::BORDER);
    DrawMagnifier(m, p.flt.left + DP(15), (p.flt.top + p.flt.bottom) / 2 - DP(1),
                  fieldFocus ? Theme::ACCENT : Theme::TEXT_FAINT);
}
static void DrawEmpty(HDC m, RECT body, const wchar_t* title, const wchar_t* sub, bool withBtn) {
    int w = body.right - body.left, h = body.bottom - body.top;
    if (w < DP(140) || h < DP(110)) return;
    int icon = DP(40), titleH = DP(22), subH = DP(56), btnH = DP(32);
    int total = icon + DP(12) + titleH + DP(4) + (sub && *sub ? subH : 0) + (withBtn ? DP(10) + btnH : 0);
    int y = body.top + (std::max)(DP(12), (h - total) / 2 - DP(8));
    int cx = (body.left + body.right) / 2;
    DrawRing(m, cx, y + icon / 2, DP(17), DP(2), Theme::BORDER);
    PIC_FillCircle(m, cx, y + icon / 2, DP(4), Theme::BORDER, Theme::BORDER);
    y += icon + DP(12);
    SetBkMode(m, TRANSPARENT);
    HFONT of = (HFONT)SelectObject(m, g_fontBold);
    SetTextColor(m, Theme::TEXT);
    RECT tt = {body.left + DP(16), y, body.right - DP(16), y + titleH};
    DrawTextW(m, title, -1, &tt, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    y += titleH + DP(4);
    if (sub && *sub) {
        SelectObject(m, g_fontBody);
        SetTextColor(m, Theme::TEXT_MUTED);
        RECT st = {body.left + DP(24), y, body.right - DP(24), y + subH};
        DrawTextW(m, sub, -1, &st, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
        y += subH;
    }
    if (withBtn) {
        y += DP(10);
        int bw = DP(150);
        g_rcConnect = RECT{cx - bw / 2, y, cx + bw / 2, y + btnH};
        COLORREF bc = g_hotConnect ? PIC_Lighten(Theme::ACCENT, 30) : Theme::ACCENT;
        PIC_FillRounded(m, g_rcConnect, DP(16), bc, bc);
        SelectObject(m, g_fontBold);
        SetTextColor(m, RGB(255, 255, 255));
        DrawTextW(m, L"Connect", -1, &g_rcConnect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(m, of);
}

// ============================================================
//                       main paint
// ============================================================
static void PaintStatusBar(HDC m, const RECT& cr) {
    FillC(m, g_rcStatus, Theme::BG_STATUS);
    HLine(m, 0, cr.right, g_rcStatus.top, Theme::BORDER);

    COLORREF sc = g_connected ? Theme::OK : Theme::DANGER;
    int dcx = (g_rcDot.left + g_rcDot.right) / 2, dcy = (g_rcDot.top + g_rcDot.bottom) / 2;
    if (g_connected) {
        double ph = (GetTickCount() % 1800) / 1800.0;
        int rr = DP(4) + (int)(ph * DP(6));
        DrawRing(m, dcx, dcy, rr, 1, Mix(sc, Theme::BG_STATUS, ph));
    }
    PIC_FillCircle(m, dcx, dcy, DP(4), sc, sc);

    // Right-hand badges first, then the message gets whatever space is left,
    // so the message can never be drawn underneath them.
    SelectObject(m, g_fontSmall);
    SetBkMode(m, TRANSPARENT);
    int rightEdge = cr.right - DP(12);
    int top = g_rcStatus.top + DP(1), bottom = g_rcStatus.bottom;
    SIZE sz{};

    if (cr.right > DP(720)) {
        const wchar_t* hint = L"F5 Refresh   Ctrl+F Search";
        GetTextExtentPoint32W(m, hint, (int)wcslen(hint), &sz);
        RECT hr = {rightEdge - sz.cx, top, rightEdge, bottom};
        SetTextColor(m, Theme::TEXT_FAINT);
        DrawTextW(m, hint, -1, &hr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        rightEdge -= sz.cx + DP(18);
    }
    if (g_live) {
        std::wstring lv = L"LIVE " + IntervalText(g_liveInterval);
        GetTextExtentPoint32W(m, lv.c_str(), (int)lv.size(), &sz);
        RECT lr = {rightEdge - sz.cx, top, rightEdge, bottom};
        SetTextColor(m, Theme::ACCENT);
        DrawTextW(m, lv.c_str(), -1, &lr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        rightEdge -= sz.cx + DP(18);
    }
    RECT st = {DP(28), top, rightEdge, bottom};
    if (st.right > st.left) {
        SetTextColor(m, Theme::TEXT_MUTED);
        DrawTextW(m, g_statusText.c_str(), -1, &st,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
}

void PaintMain(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
    RECT cr; GetClientRect(hwnd, &cr);
    RECT pr = ps.rcPaint;
    int pw = pr.right - pr.left, ph = pr.bottom - pr.top;
    if (pw <= 0 || ph <= 0) { EndPaint(hwnd, &ps); return; }
    HDC m = CreateCompatibleDC(dc);
    HBITMAP bm = CreateCompatibleBitmap(dc, pw, ph);
    HGDIOBJ obm = SelectObject(m, bm);
    SetViewportOrgEx(m, -pr.left, -pr.top, nullptr);
    FillC(m, cr, Theme::BG_DARK);
    SetBkMode(m, TRANSPARENT);
    g_rcConnect = RECT{0, 0, 0, 0};

    // ---- sidebar (no logo block) ----
    FillC(m, g_rcSide, Theme::BG_SIDE);
    VLine(m, g_rcSide.right - 1, 0, g_rcSide.bottom, Theme::BORDER);
    static const wchar_t* kLabels[SIDE_ITEMS] = {L"Explorer", L"Properties", L"Refresh", L"Live"};
    for (int i = 0; i < SIDE_ITEMS; ++i) {
        RECT ir = g_rcSideItem[i];
        bool active = (i == 0 && g_showExplorer) || (i == 1 && g_showProps) || (i == 3 && g_live);
        bool hot = (g_sideHot == i);
        RECT hr = {ir.left + DP(6), ir.top + DP(2), ir.right - DP(6), ir.bottom - DP(2)};
        COLORREF tint = Mix(Theme::BG_SIDE, Theme::ACCENT, hot ? 0.20 : 0.13);
        if (active) PIC_FillRounded(m, hr, DP(12), tint, tint);
        else if (hot) PIC_FillRounded(m, hr, DP(12), Theme::HOVER, Theme::HOVER);
        if (active) { RECT ab = {0, ir.top + DP(10), DP(3), ir.bottom - DP(10)}; FillC(m, ab, Theme::ACCENT); }
        COLORREF c = active ? Theme::ACCENT : (hot ? Theme::TEXT : Theme::TEXT_MUTED);
        int icy = g_compact ? (ir.top + ir.bottom) / 2 : ir.top + DP(22);
        DrawSideIcon(m, i, (ir.left + ir.right) / 2, icy, c);
        if (!g_compact) {
            HFONT of = (HFONT)SelectObject(m, g_fontSmall);
            SetTextColor(m, c);
            RECT lt = {ir.left, ir.top + DP(36), ir.right, ir.top + DP(54)};
            DrawTextW(m, kLabels[i], -1, &lt, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(m, of);
        }
    }

    // ---- panes ----
    HWND fw = GetFocus();
    bool exFocus = (fw == g_tree || fw == g_edit);
    bool prFocus = (fw == g_props || fw == g_propFilter);
    PaintPane(m, g_ex, L"Explorer",   g_edit,       exFocus, L"", HB_EX_COLLAPSE);
    PaintPane(m, g_pr, L"Properties", g_propFilter, prFocus, g_propSub, HB_PR_COLLAPSE);

    auto hasText = [](HWND e) { return e && GetWindowTextLengthW(e) > 0; };
    if (g_showExplorer && g_ex.all.right > g_ex.all.left && hasText(g_edit))
        DrawClear(m, ClearRect(g_ex), g_hotClear == 0);
    if (g_showProps && g_pr.all.right > g_pr.all.left && hasText(g_propFilter))
        DrawClear(m, ClearRect(g_pr), g_hotClear == 1);
    for (int i = 0; i < HB_COUNT; ++i) DrawHdrBtn(m, i);

    // ---- empty states ----
    if (g_showExplorer && g_ex.body.bottom > g_ex.body.top && !g_connected) {
        DrawEmpty(m, g_ex.body, L"Roblox not connected",
                  g_lastErr.empty()
                      ? L"Open Roblox and run Omni as Administrator. Omni connects automatically."
                      : g_lastErr.c_str(),
                  true);
    }
    if (g_showProps && g_pr.body.bottom > g_pr.body.top && g_propRows.empty()) {
        if (!g_connected)
            DrawEmpty(m, g_pr.body, L"No data", L"", false);
        else if (!g_allProps.empty())
            DrawEmpty(m, g_pr.body, L"No matching properties", L"Nothing matches your search.", false);
        else
            DrawEmpty(m, g_pr.body, L"No instance selected",
                      L"Select an item in the Explorer to see its properties.", false);
    }

    // ---- splitter ----
    if (g_rcSplit.right > g_rcSplit.left) {
        FillC(m, g_rcSplit, Theme::BG_DARK);
        int gl = DP(28), gt = DP(3);
        int cx = (g_rcSplit.left + g_rcSplit.right) / 2, cy = (g_rcSplit.top + g_rcSplit.bottom) / 2;
        RECT g = g_stacked ? RECT{cx - gl / 2, cy - gt / 2, cx + gl / 2, cy - gt / 2 + gt}
                           : RECT{cx - gt / 2, cy - gl / 2, cx - gt / 2 + gt, cy + gl / 2};
        COLORREF gc = g_dragSplit ? Theme::ACCENT : Theme::BORDER;
        PIC_FillRounded(m, g, gt, gc, gc);
    }

    PaintStatusBar(m, cr);

    BitBlt(dc, pr.left, pr.top, pw, ph, m, pr.left, pr.top, SRCCOPY);
    SelectObject(m, obm); DeleteObject(bm); DeleteDC(m);
    EndPaint(hwnd, &ps);
}
