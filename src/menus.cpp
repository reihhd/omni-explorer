#include "menus.h"
#include "state.h"
#include "util.h"
#include "memory.h"
#include "tree.h"
#include "props.h"
#include "paint.h"
#include "live.h"

void ShowTreeMenu(HTREEITEM item, POINT pt) {
    uintptr_t a = ItemAddr(item);
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_NAME,  L"Copy Name");
    AppendMenuW(m, MF_STRING, IDM_CLASS, L"Copy ClassName");
    AppendMenuW(m, MF_STRING, IDM_ADDR,  L"Copy Address");
    AppendMenuW(m, MF_STRING, IDM_PATH,  L"Copy Path");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, IDM_EXPAND,   L"Expand");
    AppendMenuW(m, MF_STRING, IDM_COLLAPSE, L"Collapse");
    AppendMenuW(m, MF_STRING, IDM_RELOAD,   L"Reload children");
    int cmd = (int)TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, g_main, nullptr);
    DestroyMenu(m);
    std::wstring s;
    switch (cmd) {
    case IDM_NAME:  s = W(InstName(a));   CopyText(s); Toast(s); break;
    case IDM_CLASS: s = W(ClassName(a));  CopyText(s); Toast(s); break;
    case IDM_ADDR:  s = Hex(a);           CopyText(s); Toast(s); break;
    case IDM_PATH:  s = PathString(item); CopyText(s); Toast(s); break;
    case IDM_EXPAND:   TreeView_Expand(g_tree, item, TVE_EXPAND); break;
    case IDM_COLLAPSE: TreeView_Expand(g_tree, item, TVE_COLLAPSE); break;
    case IDM_RELOAD:   ReloadNode(item); break;
    }
}

void ShowPropMenu(int row, POINT pt) {
    if (row < 0 || row >= (int)g_propRows.size()) return;
    const PropRow& r = g_propRows[row];
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_VAL,   L"Copy Value");
    AppendMenuW(m, MF_STRING, IDM_PNAME, L"Copy Name");
    AppendMenuW(m, MF_STRING | (r.offset.empty() ? MF_GRAYED : 0), IDM_POFF, L"Copy Offset");
    AppendMenuW(m, MF_STRING, IDM_PPAIR, L"Copy Name = Value");
    int cmd = (int)TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, g_main, nullptr);
    DestroyMenu(m);
    std::wstring nm = r.name;
    size_t d = nm.find(L'.');
    if (d != std::wstring::npos) nm = nm.substr(d + 1);
    std::wstring s;
    switch (cmd) {
    case IDM_VAL:   s = r.text;   break;
    case IDM_PNAME: s = nm;       break;
    case IDM_POFF:  s = r.offset; break;
    case IDM_PPAIR: s = nm + L" = " + r.text; break;
    default: return;
    }
    CopyText(s); Toast(s);
}

void ShowLiveMenu(POINT pt) {
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING | (g_live ? MF_CHECKED : 0), IDM_LIVE_TOGGLE, L"Live refresh");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    for (int i = 0; i < kLiveIntervalCount; ++i) {
        std::wstring label = L"Every " + IntervalText(kLiveIntervals[i]);
        AppendMenuW(m, MF_STRING | (g_liveInterval == kLiveIntervals[i] ? MF_CHECKED : 0),
                    IDM_INT_BASE + i, label.c_str());
    }
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING | (g_autoTree ? MF_CHECKED : 0), IDM_TREE_SYNC, L"Also sync Explorer tree");
    int cmd = (int)TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, g_main, nullptr);
    DestroyMenu(m);
    if (cmd == IDM_LIVE_TOGGLE) SetLive(!g_live);
    else if (cmd == IDM_TREE_SYNC) {
        g_autoTree = !g_autoTree;
        SetStatus(g_autoTree ? L"Explorer sync on" : L"Explorer sync off");
    }
    else if (cmd >= IDM_INT_BASE && cmd < IDM_INT_BASE + kLiveIntervalCount)
        SetLiveInterval(kLiveIntervals[cmd - IDM_INT_BASE]);
}
