#include "layout.h"
#include "state.h"
#include "util.h"
#include "props.h"

RECT g_rcSide{}, g_rcStatus{}, g_rcDot{}, g_rcArea{}, g_rcSplit{}, g_rcConnect{};
RECT g_rcSideItem[SIDE_ITEMS]{};
RECT g_hb[HB_COUNT];
PaneRects g_ex, g_pr;

static void MakePane(PaneRects& p, RECT all) {
    p.all = all;
    p.hdr = {all.left, all.top, all.right, all.top + DP(32)};
    p.flt = {all.left + DP(8), p.hdr.bottom + DP(7), all.right - DP(8), p.hdr.bottom + DP(7) + DP(28)};
    p.body = {all.left, p.flt.bottom + DP(7), all.right, all.bottom};
    if (p.body.bottom < p.body.top) p.body.bottom = p.body.top;
}

void ComputeLayout(int cw, int ch) {
    g_compact = cw < DP(880);
    const int sideW = g_compact ? DP(52) : DP(72);
    const int stH = DP(26), sp = DP(SPLIT_W);
    g_rcSide   = {0, 0, sideW, ch - stH};
    g_rcStatus = {0, ch - stH, cw, ch};
    int dy = ch - stH + (stH - DP(8)) / 2;
    g_rcDot    = {DP(12), dy, DP(12) + DP(8), dy + DP(8)};

    // sidebar items start at the top now (no logo block any more)
    int itemH = g_compact ? DP(46) : DP(58);
    int iy = DP(10);
    for (int i = 0; i < SIDE_ITEMS; ++i) {
        g_rcSideItem[i] = {0, iy, sideW, iy + itemH};
        iy += itemH;
        if (i == 1) iy += DP(10);          // small gap between view toggles and actions
    }

    g_rcArea = {sideW, 0, cw, ch - stH};
    g_ex = PaneRects{}; g_pr = PaneRects{}; g_rcSplit = {0, 0, 0, 0};

    int aw = g_rcArea.right - g_rcArea.left, ah = g_rcArea.bottom - g_rcArea.top;
    g_stacked = aw < DP(760);
    if (g_showExplorer && g_showProps) {
        if (!g_stacked) {
            int total = aw - sp;
            int lw = (int)(total * g_split);
            lw = (std::max)(DP(180), (std::min)(lw, total - DP(240)));
            MakePane(g_ex, RECT{g_rcArea.left, 0, g_rcArea.left + lw, ah});
            g_rcSplit = {g_rcArea.left + lw, 0, g_rcArea.left + lw + sp, ah};
            MakePane(g_pr, RECT{g_rcSplit.right, 0, g_rcArea.right, ah});
        } else {
            int total = ah - sp;
            int th = (int)(total * g_split);
            th = (std::max)(DP(150), (std::min)(th, total - DP(150)));
            MakePane(g_ex, RECT{g_rcArea.left, 0, g_rcArea.right, th});
            g_rcSplit = {g_rcArea.left, th, g_rcArea.right, th + sp};
            MakePane(g_pr, RECT{g_rcArea.left, g_rcSplit.bottom, g_rcArea.right, ah});
        }
    } else if (g_showExplorer) {
        MakePane(g_ex, g_rcArea);
    } else {
        MakePane(g_pr, g_rcArea);
    }
}

void ComputeHeaderButtons() {
    for (auto& r : g_hb) r = RECT{0, 0, 0, 0};
    auto place = [&](const PaneRects& p, std::initializer_list<int> ids) {
        int bs = DP(24), gap = DP(2);
        int x = p.hdr.right - DP(6);
        int y = p.hdr.top + ((p.hdr.bottom - p.hdr.top) - bs) / 2;
        for (int id : ids) { g_hb[id] = RECT{x - bs, y, x, y + bs}; x -= bs + gap; }
    };
    if (g_showExplorer && g_ex.all.right > g_ex.all.left) place(g_ex, {HB_EX_REFRESH, HB_EX_COLLAPSE});
    if (g_showProps    && g_pr.all.right > g_pr.all.left) place(g_pr, {HB_PR_LIVE, HB_PR_COLLAPSE});
}

void ResizePropColumns() {
    if (!g_props) return;
    RECT rc; GetClientRect(g_props, &rc);
    int w = rc.right - rc.left;
    if (w <= 0) return;
    int offW = (w >= DP(420)) ? DP(92) : 0;
    int nameW = (w - offW) * 42 / 100;
    if (nameW < DP(100)) nameW = DP(100);
    int valW = w - offW - nameW;
    if (valW < DP(60)) valW = DP(60);
    ListView_SetColumnWidth(g_props, 0, nameW);
    ListView_SetColumnWidth(g_props, 1, valW);
    ListView_SetColumnWidth(g_props, 2, offW);
}

void UpdateVisibility() {
    if (!g_tree) return;
    bool exOk = g_showExplorer && g_connected;
    bool prOk = g_showProps && g_connected && !g_propRows.empty();
    bool changed = false;
    auto set = [&](HWND h, bool v) {
        bool cur = (GetWindowLongW(h, GWL_STYLE) & WS_VISIBLE) != 0;
        if (cur != v) { ShowWindow(h, v ? SW_SHOW : SW_HIDE); changed = true; }
    };
    set(g_tree, exOk);
    set(g_edit, g_showExplorer);
    set(g_props, prOk);
    set(g_propFilter, g_showProps);
    if (changed && g_main) InvalidateRect(g_main, nullptr, FALSE);
}

static const UINT kTtSize = (UINT)offsetof(TTTOOLINFOW, lParam);
static bool g_tipsAdded = false;

void UpdateTips() {
    if (!g_tip) return;
    RECT none = {0, 0, 0, 0};
    struct T { int id; RECT r; const wchar_t* text; };
    T tips[] = {
        {0,  g_compact ? g_rcSideItem[0] : none, L"Explorer"},
        {1,  g_compact ? g_rcSideItem[1] : none, L"Properties"},
        {2,  g_compact ? g_rcSideItem[2] : none, L"Refresh (F5)"},
        {3,  g_compact ? g_rcSideItem[3] : none, L"Live refresh (right-click for options)"},
        {10, g_hb[HB_EX_COLLAPSE], L"Collapse all"},
        {11, g_hb[HB_EX_REFRESH],  L"Refresh (F5)"},
        {12, g_hb[HB_PR_COLLAPSE], L"Collapse / expand all groups"},
        {13, g_hb[HB_PR_LIVE],     L"Live refresh (right-click for options)"},
    };
    for (auto& t : tips) {
        TTTOOLINFOW ti{};
        ti.cbSize = kTtSize; ti.hwnd = g_main; ti.uId = (UINT_PTR)t.id; ti.rect = t.r;
        if (!g_tipsAdded) {
            ti.uFlags = TTF_SUBCLASS; ti.lpszText = (LPWSTR)t.text;
            SendMessageW(g_tip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        } else {
            SendMessageW(g_tip, TTM_NEWTOOLRECTW, 0, (LPARAM)&ti);
        }
    }
    g_tipsAdded = true;
}

void Layout(int cw, int ch) {
    if (!g_tree) return;
    ComputeLayout(cw, ch);
    ComputeHeaderButtons();

    auto field = [&](HWND h, const RECT& r) {
        int eh = DP(18), y = r.top + ((r.bottom - r.top) - eh) / 2;
        MoveWindow(h, r.left + DP(32), y, (std::max)(10, (int)((r.right - r.left) - DP(32) - DP(30))), eh, TRUE);
    };
    if (g_showExplorer) {
        field(g_edit, g_ex.flt);
        MoveWindow(g_tree, g_ex.body.left, g_ex.body.top,
                   g_ex.body.right - g_ex.body.left, g_ex.body.bottom - g_ex.body.top, TRUE);
    }
    if (g_showProps) {
        field(g_propFilter, g_pr.flt);
        MoveWindow(g_props, g_pr.body.left, g_pr.body.top,
                   g_pr.body.right - g_pr.body.left, g_pr.body.bottom - g_pr.body.top, TRUE);
    }
    ResizePropColumns();
    UpdateVisibility();
    UpdateTips();
}

void RelayoutNow() {
    RECT cr; GetClientRect(g_main, &cr);
    Layout(cr.right, cr.bottom);
    InvalidateRect(g_main, nullptr, FALSE);
}
