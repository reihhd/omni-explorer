#include "tree.h"
#include "state.h"
#include "util.h"
#include "gfx.h"
#include "theme.h"
#include "memory.h"
#include "classmeta.h"
#include "process.h"
#include "layout.h"
#include "paint.h"

// ============================================================
//                       basic node helpers
// ============================================================
uintptr_t ItemAddr(HTREEITEM h) {
    TVITEMW it{}; it.mask = TVIF_PARAM; it.hItem = h;
    TreeView_GetItem(g_tree, &it);
    return (uintptr_t)it.lParam;
}
// Dex style: only the name (the class is shown by the icon and the right-hand column)
std::wstring Label(uintptr_t a) {
    return W(InstName(a));
}
HTREEITEM InsertNode(HTREEITEM parent, uintptr_t addr, const std::wstring& text, bool kids) {
    int icon = IconForClass(ClassName(addr));
    TVINSERTSTRUCTW t{};
    t.hParent = parent;
    t.hInsertAfter = TVI_LAST;
    t.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    t.item.pszText = (LPWSTR)text.c_str();
    t.item.lParam = (LPARAM)addr;
    t.item.cChildren = kids ? 1 : 0;
    t.item.iImage = icon;
    t.item.iSelectedImage = icon;
    return TreeView_InsertItem(g_tree, &t);
}
void PopulateChildren(HTREEITEM item, uintptr_t addr) {
    std::vector<uintptr_t> kids = ChildrenStable(addr);
    if (kids.empty()) {
        TVITEMW ti{}; ti.mask = TVIF_CHILDREN; ti.hItem = item; ti.cChildren = 0;
        TreeView_SetItem(g_tree, &ti);
        return;
    }
    for (uintptr_t c : kids) {
        auto it = g_childrenCache.find(c);
        bool has;
        if (it != g_childrenCache.end()) has = !it->second.empty();
        else has = HasChildrenCached(c);
        InsertNode(item, c, Label(c), has);
    }
}

// ============================================================
//                       save / restore state
// ============================================================
std::vector<std::string> GetNodePath(HTREEITEM item) {
    std::vector<std::string> parts;
    while (item) {
        parts.push_back(InstName(ItemAddr(item)));
        item = TreeView_GetParent(g_tree, item);
    }
    std::reverse(parts.begin(), parts.end());
    return parts;
}
static void CollectExpanded(HTREEITEM item, std::vector<std::vector<std::string>>& out) {
    while (item) {
        if (TreeView_GetItemState(g_tree, item, TVIS_EXPANDED) & TVIS_EXPANDED)
            out.push_back(GetNodePath(item));
        HTREEITEM child = TreeView_GetChild(g_tree, item);
        if (child) CollectExpanded(child, out);
        item = TreeView_GetNextSibling(g_tree, item);
    }
}
void SaveTreeState() {
    g_savedExpanded.clear();
    g_savedSelected.clear();
    if (!g_tree) return;
    HTREEITEM sel = TreeView_GetSelection(g_tree);
    if (sel) g_savedSelected = GetNodePath(sel);
    HTREEITEM root = TreeView_GetRoot(g_tree);
    if (root) CollectExpanded(root, g_savedExpanded);
}
static HTREEITEM FindChildByName(HTREEITEM parent, const std::string& name) {
    HTREEITEM c = TreeView_GetChild(g_tree, parent);
    while (c) {
        if (InstName(ItemAddr(c)) == name) return c;
        c = TreeView_GetNextSibling(g_tree, c);
    }
    return nullptr;
}
static HTREEITEM FindByPath(const std::vector<std::string>& path) {
    HTREEITEM cur = TreeView_GetRoot(g_tree);
    if (!cur || path.empty()) return nullptr;
    if (InstName(ItemAddr(cur)) != path[0]) return nullptr;
    for (size_t i = 1; i < path.size(); ++i) {
        if (!(TreeView_GetItemState(g_tree, cur, TVIS_EXPANDED) & TVIS_EXPANDED)) {
            if (!g_populated.count(cur)) {
                g_populated.insert(cur);
                PopulateChildren(cur, ItemAddr(cur));
            }
            TreeView_Expand(g_tree, cur, TVE_EXPAND);
        }
        HTREEITEM c = FindChildByName(cur, path[i]);
        if (!c) return nullptr;
        cur = c;
    }
    return cur;
}
void RestoreTreeState() {
    std::vector<std::vector<std::string>> sorted = g_savedExpanded;
    std::sort(sorted.begin(), sorted.end(),
              [](const std::vector<std::string>& a, const std::vector<std::string>& b) {
                  return a.size() < b.size();
              });
    for (auto& p : sorted) {
        HTREEITEM h = FindByPath(p);
        if (h) {
            if (!g_populated.count(h)) {
                g_populated.insert(h);
                PopulateChildren(h, ItemAddr(h));
            }
            TreeView_Expand(g_tree, h, TVE_EXPAND);
        }
    }
    if (!g_savedSelected.empty()) {
        HTREEITEM h = FindByPath(g_savedSelected);
        if (h) { TreeView_SelectItem(g_tree, h); TreeView_EnsureVisible(g_tree, h); }
    }
}

void LoadRoot(bool preserveState) {
    if (preserveState) SaveTreeState();
    else { g_savedExpanded.clear(); g_savedSelected.clear(); }
    TreeView_DeleteAllItems(g_tree);
    g_populated.clear();
    g_childrenCache.clear();
    g_classCache.clear();
    uintptr_t dm = GetDataModel();
    if (!IsUserPtr(dm)) {
        g_connected = false;
        g_dm = 0;
        g_lastErr = L"DataModel not ready \u2014 waiting for the game to finish loading\u2026";
        SetWindowTextW(g_main, L"Omni");
        SetStatus(g_lastErr);
        UpdateVisibility();
        InvalidateRect(g_main, nullptr, FALSE);
        return;
    }
    g_connected = true;
    g_dm = dm;
    g_lastErr.clear();
    HTREEITEM root = InsertNode(TVI_ROOT, dm, L"game", true);
    TreeView_SelectItem(g_tree, root);
    TreeView_Expand(g_tree, root, TVE_EXPAND);
    if (preserveState) RestoreTreeState();
    wchar_t t[512];
    swprintf(t, 512, L"Omni \u2014 PID %u \u00B7 DataModel 0x%llX", (unsigned)g_pid, (unsigned long long)dm);
    SetWindowTextW(g_main, t);
    wchar_t s[256];
    swprintf(s, 256, L"Connected \u00B7 PID %u \u00B7 DataModel 0x%llX", (unsigned)g_pid, (unsigned long long)dm);
    SetStatus(std::wstring(s) + VersionNote());
    UpdateVisibility();
    InvalidateRect(g_main, nullptr, FALSE);
}

// ============================================================
//                  path / reload / collapse
// ============================================================
static bool IsIdent(const std::string& s) {
    if (s.empty() || isdigit((unsigned char)s[0])) return false;
    for (unsigned char c : s) if (!(isalnum(c) || c == '_')) return false;
    return true;
}
std::wstring PathString(HTREEITEM item) {
    std::vector<std::string> parts = GetNodePath(item);
    std::string out = "game";
    for (size_t i = 1; i < parts.size(); ++i) {
        if (IsIdent(parts[i])) out += "." + parts[i];
        else {
            std::string e;
            for (char c : parts[i]) { if (c == '"' || c == '\\') e += '\\'; e += c; }
            out += "[\"" + e + "\"]";
        }
    }
    return W(out);
}
static void ForgetSubtree(HTREEITEM it) {
    for (HTREEITEM c = TreeView_GetChild(g_tree, it); c; c = TreeView_GetNextSibling(g_tree, c)) {
        g_populated.erase(c);
        g_childrenCache.erase(ItemAddr(c));
        ForgetSubtree(c);
    }
}
void ReloadNode(HTREEITEM item) {
    uintptr_t a = ItemAddr(item);
    ForgetSubtree(item);
    g_populated.erase(item);
    g_childrenCache.erase(a);
    TreeView_Expand(g_tree, item, TVE_COLLAPSE | TVE_COLLAPSERESET);
    TVITEMW ti{}; ti.mask = TVIF_CHILDREN; ti.hItem = item; ti.cChildren = 1;
    TreeView_SetItem(g_tree, &ti);
    TreeView_Expand(g_tree, item, TVE_EXPAND);
}
static void CollapseRec(HTREEITEM item) {
    for (HTREEITEM c = TreeView_GetChild(g_tree, item); c; c = TreeView_GetNextSibling(g_tree, c)) {
        if (TreeView_GetItemState(g_tree, c, TVIS_EXPANDED) & TVIS_EXPANDED) {
            CollapseRec(c);
            TreeView_Expand(g_tree, c, TVE_COLLAPSE);
        }
    }
}
void CollapseAllTree() {
    HTREEITEM root = TreeView_GetRoot(g_tree);
    if (!root) return;
    CollapseRec(root);
    TreeView_SelectItem(g_tree, root);
    TreeView_EnsureVisible(g_tree, root);
}

// ============================================================
//                  live sync of expanded branches
// ============================================================
static bool g_syncChanged = false;
static std::unordered_map<uintptr_t, int> g_emptyStrikes;

void SyncNode(HTREEITEM item) {
    uintptr_t a = ItemAddr(item);
    std::vector<uintptr_t> fresh = ReadChildrenRaw(a);   // always read fresh memory

    std::unordered_map<uintptr_t, HTREEITEM> cur;
    for (HTREEITEM c = TreeView_GetChild(g_tree, item); c; c = TreeView_GetNextSibling(g_tree, c))
        cur[ItemAddr(c)] = c;

    // An empty read while we still show children may be a transient failure: require 2 in a row.
    if (fresh.empty() && !cur.empty()) {
        if (++g_emptyStrikes[a] < 2) return;
    }
    g_emptyStrikes.erase(a);

    std::unordered_set<uintptr_t> freshSet(fresh.begin(), fresh.end());
    bool changed = false;

    for (auto& kv : cur) {                       // removed instances
        if (freshSet.count(kv.first)) continue;
        ForgetSubtree(kv.second);
        g_populated.erase(kv.second);
        g_childrenCache.erase(kv.first);
        TreeView_DeleteItem(g_tree, kv.second);
        changed = true;
    }
    for (uintptr_t c : fresh) {                  // new instances
        if (cur.count(c)) continue;
        g_classCache.erase(c);
        g_childrenCache.erase(c);
        bool has = HasChildrenCached(c);
        cur[c] = InsertNode(item, c, Label(c), has);
        changed = true;
    }
    g_childrenCache[a] = fresh;

    if (changed) {
        TVITEMW ti{}; ti.mask = TVIF_CHILDREN; ti.hItem = item; ti.cChildren = fresh.empty() ? 0 : 1;
        TreeView_SetItem(g_tree, &ti);
        g_syncChanged = true;
    }

    for (uintptr_t c : fresh) {                  // renamed instances
        HTREEITEM h = cur[c];
        std::wstring want = Label(c);
        wchar_t buf[1100] = {0};
        TVITEMW t{}; t.mask = TVIF_TEXT; t.hItem = h; t.pszText = buf; t.cchTextMax = 1100;
        if (TreeView_GetItem(g_tree, &t) && want != buf) {
            t.pszText = (LPWSTR)want.c_str();
            TreeView_SetItem(g_tree, &t);
            g_syncChanged = true;
        }
    }
    for (uintptr_t c : fresh) {                  // recurse into expanded, already-populated children
        HTREEITEM h = cur[c];
        if (g_populated.count(h) && (TreeView_GetItemState(g_tree, h, TVIS_EXPANDED) & TVIS_EXPANDED))
            SyncNode(h);
    }
}
void SyncTree() {
    if (!g_tree || !g_connected || g_filterActive) return;
    HTREEITEM root = TreeView_GetRoot(g_tree);
    if (!root) return;
    g_syncChanged = false;
    SendMessageW(g_tree, WM_SETREDRAW, FALSE, 0);
    SyncNode(root);
    SendMessageW(g_tree, WM_SETREDRAW, TRUE, 0);
    if (g_syncChanged) InvalidateRect(g_tree, nullptr, TRUE);
}

// ============================================================
//                  custom draw
// ============================================================
// Full-width rows, accent bar on the left when selected, faint class name on the right.
LRESULT HandleTreeCustomDraw(LPNMTVCUSTOMDRAW cd) {
    switch (cd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT: {
        HTREEITEM it = (HTREEITEM)cd->nmcd.dwItemSpec;
        bool sel = (cd->nmcd.uItemState & CDIS_SELECTED) != 0;
        bool hot = (cd->nmcd.uItemState & CDIS_HOT) != 0;
        RECT r;
        if (!TreeView_GetItemRect(g_tree, it, &r, FALSE)) return CDRF_DODEFAULT;
        RECT cr; GetClientRect(g_tree, &cr);
        r.left = 0; r.right = cr.right;
        COLORREF bg = sel ? Theme::SEL : (hot ? Theme::HOVER : Theme::BG_PANEL);
        FillC(cd->nmcd.hdc, r, bg);
        if (sel) { RECT bar = {0, r.top, DP(3), r.bottom}; FillC(cd->nmcd.hdc, bar, Theme::ACCENT); }
        cd->clrTextBk = bg;
        cd->clrText = sel ? RGB(255, 255, 255) : Theme::TEXT;
        cd->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS | CDIS_HOT);
        return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
    }
    case CDDS_ITEMPOSTPAINT: {
        HDC hdc = cd->nmcd.hdc;
        HTREEITEM it = (HTREEITEM)cd->nmcd.dwItemSpec;
        RECT tr, cr, row;
        if (!TreeView_GetItemRect(g_tree, it, &tr, TRUE)) return CDRF_DODEFAULT;
        if (!TreeView_GetItemRect(g_tree, it, &row, FALSE)) return CDRF_DODEFAULT;
        GetClientRect(g_tree, &cr);
        if (cr.right < DP(260)) return CDRF_DODEFAULT;
        std::string cls = ClassName((uintptr_t)cd->nmcd.lItemlParam);
        if (cls.empty() || cls == "<unknown>") return CDRF_DODEFAULT;
        std::wstring wc = W(cls);
        bool sel = (TreeView_GetItemState(g_tree, it, TVIS_SELECTED) & TVIS_SELECTED) != 0;
        int saved = SaveDC(hdc);
        IntersectClipRect(hdc, 0, row.top, cr.right, row.bottom);        // stay inside this row
        SelectObject(hdc, g_fontSmall);
        SIZE sz{}; GetTextExtentPoint32W(hdc, wc.c_str(), (int)wc.size(), &sz);
        int right = cr.right - DP(10), left = right - sz.cx;
        if (left > tr.right + DP(16)) {      // only when it can't collide with the name
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, sel ? RGB(190, 215, 245) : Theme::TEXT_FAINT);
            RECT t = {left, row.top, right, row.bottom};
            DrawTextW(hdc, wc.c_str(), -1, &t, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        RestoreDC(hdc, saved);
        return CDRF_DODEFAULT;
    }
    }
    return CDRF_DODEFAULT;
}

// ============================================================
//                       search filter
// ============================================================
static void BuildFilteredTree(const std::string& query) {
    uintptr_t dm = GetDataModel();
    if (!IsUserPtr(dm)) return;
    std::string lq = Lower(query);
    if (lq.empty()) return;

    SetCursor(LoadCursor(nullptr, IDC_WAIT));
    std::unordered_map<uintptr_t, uintptr_t> parentMap;
    std::vector<uintptr_t> allNodes;
    std::vector<uintptr_t> stack;
    stack.push_back(dm);
    size_t scanned = 0;
    while (!stack.empty() && scanned < MAX_SCAN_NODES) {
        uintptr_t a = stack.back(); stack.pop_back();
        allNodes.push_back(a); ++scanned;
        auto kids = Children(a);      // single read, no retry/sleep, so searching stays fast
        for (uintptr_t c : kids) { parentMap[c] = a; stack.push_back(c); }
    }
    std::unordered_set<uintptr_t> visible;
    size_t matchCount = 0;
    for (uintptr_t a : allNodes) {
        std::string nm = Lower(InstName(a));
        std::string cls = Lower(ClassName(a));
        if (nm.find(lq) != std::string::npos || cls.find(lq) != std::string::npos) {
            ++matchCount;
            uintptr_t cur = a;
            while (cur) {
                visible.insert(cur);
                auto it = parentMap.find(cur);
                if (it == parentMap.end()) break;
                cur = it->second;
            }
        }
    }
    TreeView_DeleteAllItems(g_tree);
    g_populated.clear();
    std::function<void(uintptr_t, HTREEITEM)> insertVisible =
        [&](uintptr_t a, HTREEITEM parent) {
            auto kids = Children(a);
            std::vector<uintptr_t> vkids;
            for (uintptr_t c : kids) if (visible.count(c)) vkids.push_back(c);
            HTREEITEM item = InsertNode(parent, a,
                (parent == TVI_ROOT) ? std::wstring(L"game") : Label(a),
                !vkids.empty());
            g_populated.insert(item);      // already filled; stops TVN_ITEMEXPANDING from re-populating
            for (uintptr_t c : vkids) insertVisible(c, item);
        };
    insertVisible(dm, TVI_ROOT);
    std::function<void(HTREEITEM)> expandAll = [&](HTREEITEM item) {
        while (item) {
            if (TreeView_GetChild(g_tree, item)) {
                TreeView_Expand(g_tree, item, TVE_EXPAND);
                expandAll(TreeView_GetChild(g_tree, item));
            }
            item = TreeView_GetNextSibling(g_tree, item);
        }
    };
    expandAll(TreeView_GetRoot(g_tree));
    HTREEITEM root = TreeView_GetRoot(g_tree);
    if (root) TreeView_SelectItem(g_tree, root);
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    wchar_t s[256];
    swprintf(s, 256, L"Filter \"%s\" \u00B7 %zu matches \u00B7 %zu nodes scanned",
             W(query).c_str(), matchCount, scanned);
    SetStatus(s);
}

void ApplySearch() {
    wchar_t buf[256] = {0};
    if (g_edit) GetWindowTextW(g_edit, buf, 256);
    std::string q = Narrow(buf);
    if (q.empty()) {
        if (g_filterActive) {
            g_filterActive = false;
            TreeView_DeleteAllItems(g_tree);
            g_populated.clear();
            g_childrenCache.clear();
            g_classCache.clear();
            uintptr_t dm = GetDataModel();
            if (!IsUserPtr(dm)) return;
            HTREEITEM root = InsertNode(TVI_ROOT, dm, L"game", true);
            TreeView_SelectItem(g_tree, root);
            TreeView_Expand(g_tree, root, TVE_EXPAND);
            RestoreTreeState();
            SetStatus(L"Filter cleared");
        }
        return;
    }
    if (!g_filterActive) { SaveTreeState(); g_filterActive = true; }
    BuildFilteredTree(q);
}
