#include "live.h"
#include "state.h"
#include "util.h"
#include "memory.h"
#include "props.h"
#include "tree.h"
#include "layout.h"
#include "paint.h"

const int kLiveIntervals[kLiveIntervalCount] = {250, 500, 1000, 2000, 5000};

std::wstring IntervalText(int ms) {
    wchar_t b[32];
    if (ms < 1000) swprintf(b, 32, L"%d ms", ms);
    else if (ms % 1000 == 0) swprintf(b, 32, L"%d s", ms / 1000);
    else swprintf(b, 32, L"%.1f s", ms / 1000.0);
    return b;
}

void SetLive(bool on) {
    g_live = on;
    if (on) SetTimer(g_main, TIMER_LIVE, (UINT)g_liveInterval, nullptr);
    else    KillTimer(g_main, TIMER_LIVE);
    SetStatus(on ? L"Live on \u2014 refreshing every " + IntervalText(g_liveInterval)
                 : L"Live off");
    InvalidateRect(g_main, nullptr, FALSE);
}
void SetLiveInterval(int ms) {
    g_liveInterval = ms;
    if (g_live) SetTimer(g_main, TIMER_LIVE, (UINT)ms, nullptr);   // re-arms with the new period
    SetStatus(L"Live interval: " + IntervalText(ms));
    InvalidateRect(g_main, nullptr, FALSE);
}

// Called by TIMER_LIVE. Re-reads the selected instance and keeps the explorer in sync.
void LiveTick() {
    if (!g_live || !g_connected || !g_main || IsIconic(g_main)) return;

    // 1) explorer: add / remove / rename nodes in expanded branches (throttled to every >= 2 s)
    static DWORD lastSync = 0;
    DWORD now = GetTickCount();
    if (g_autoTree && g_showExplorer && !g_filterActive && now - lastSync >= 2000) {
        lastSync = now;
        SyncTree();
    }

    // 2) properties of the selected instance
    if (!g_showProps || g_propRows.empty()) return;
    HTREEITEM sel = TreeView_GetSelection(g_tree);
    if (!sel) return;
    uintptr_t addr = ItemAddr(sel);
    g_childrenCache.erase(addr);                 // so the "Children" count is fresh too

    std::vector<PropRow> old = g_propRows;
    BuildProps(addr);
    RebuildPropRows();

    std::wstring sub = W(InstName(addr)) + L"  (" + W(ClassName(addr)) + L")";
    if (sub != g_propSub) { g_propSub = sub; InvalidateRect(g_main, &g_pr.hdr, FALSE); }

    bool same = old.size() == g_propRows.size();
    if (same) for (size_t i = 0; i < old.size(); ++i)
        if (old[i].name != g_propRows[i].name || old[i].kind != g_propRows[i].kind) { same = false; break; }
    if (!same) { ApplyProps(true); return; }

    bool any = false;
    for (size_t i = 0; i < old.size(); ++i) {
        if (g_propRows[i].kind == PK_Group) continue;
        if (old[i].text != g_propRows[i].text) {
            g_propRows[i].changedAt = now; any = true;
            ListView_SetItemText(g_props, (int)i, 1, (LPWSTR)g_propRows[i].text.c_str());
        } else {
            g_propRows[i].changedAt = old[i].changedAt;
        }
    }
    if (any) g_flashUntil = now + 1000;
    InvalidateRect(g_props, nullptr, FALSE);
}
