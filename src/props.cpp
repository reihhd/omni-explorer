#include "props.h"
#include "state.h"
#include "util.h"
#include "gfx.h"
#include "theme.h"
#include "memory.h"
#include "classmeta.h"
#include "layout.h"
#include "tree.h"

std::vector<PropRow> g_allProps;
std::vector<PropRow> g_propRows;

// ============================================================
//                  value formatting / type detection
// ============================================================
std::wstring FmtF(float f) {
    if (std::isnan(f) || std::isinf(f)) return L"0";
    if (f == 0.0f) return L"0";
    if (std::fabs(f) < 1e-6f) return L"0";
    if (std::fabs(f) < 1.0e9f && f == (float)(int)f) {
        wchar_t b[32]; swprintf(b, 32, L"%d", (int)f); return b;
    }
    wchar_t b[64]; swprintf(b, 64, L"%.6f", (double)f);
    std::wstring s = b;
    size_t dot = s.find(L'.');
    if (dot != std::wstring::npos) {
        size_t last = s.find_last_not_of(L'0');
        if (last != std::wstring::npos && last > dot) s = s.substr(0, last + 1);
        else if (last == dot) s = s.substr(0, dot);
    }
    return s;
}
static bool EndsWith(const std::string& s, const char* suf) {
    size_t l = strlen(suf);
    return s.size() >= l && s.compare(s.size() - l, l, suf) == 0;
}

struct TypeHint { const char* cls; const char* name; PropKind kind; };
static const TypeHint kHints[] = {
    { "Model", "Scale", PK_Number },
    { "Primitive", "Flags", PK_Number }, { "Primitive", "Validate", PK_Number },
    { "Primitive", "Material", PK_Number },
    { "Primitive", "Position", PK_Vector3 }, { "Primitive", "Size", PK_Vector3 },
    { "Primitive", "Rotation", PK_Vector3 },
    { "Primitive", "AssemblyLinearVelocity", PK_Vector3 },
    { "Primitive", "AssemblyAngularVelocity", PK_Vector3 },
    { "GuiObject", "ZIndex", PK_Int }, { "GuiObject", "LayoutOrder", PK_Int },
    { "SpecialMesh", "Scale", PK_Vector3 },
    { "Humanoid", "MoveDirection", PK_Vector3 }, { "Humanoid", "CameraOffset", PK_Vector3 },
    { "Humanoid", "TargetPoint", PK_Vector3 }, { "Humanoid", "MoveToPoint", PK_Vector3 },
    { "Camera", "Position", PK_Vector3 }, { "Camera", "Rotation", PK_Vector3 },
    { "Attachment", "Position", PK_Vector3 },
    { "World", "Gravity", PK_Number },
    { "AirProperties", "AirDensity", PK_Number }, { "AirProperties", "GlobalWind", PK_Vector3 },
    { "Lighting", "Ambient", PK_Color3 }, { "Lighting", "OutdoorAmbient", PK_Color3 },
    { "Lighting", "ColorShift_Top", PK_Color3 }, { "Lighting", "ColorShift_Bottom", PK_Color3 },
    { "Lighting", "FogColor", PK_Color3 }, { "Lighting", "GradientTop", PK_Color3 },
    { "Lighting", "GradientBottom", PK_Color3 }, { "Lighting", "LightColor", PK_Color3 },
    { "Lighting", "SunPosition", PK_Vector3 }, { "Lighting", "MoonPosition", PK_Vector3 },
    { "Lighting", "LightDirection", PK_Vector3 },
};
static bool LookupHint(const std::string& cls, const std::string& name, PropKind& out) {
    for (auto& h : kHints) if (cls == h.cls && name == h.name) { out = h.kind; return true; }
    return false;
}

static const std::unordered_set<std::string> kStringProps = {
    "name","text","placeholdertext","image","texture","textureid",
    "meshid","soundid","animationid","tooltip","displayname",
    "description","title","subtitle","message","skyboxbk","skyboxdn",
    "skyboxft","skyboxlf","skyboxrt","skyboxup","suntextureid",
    "moontextureid","colormap","normalmap","roughnessmap","metalnessmap",
    "emissivemaskcontent","guid","objecttext","actiontext","icon",
    "mouseicon","cursoricon","activatedcursoricon","mousetexture",
    "assetid","imageid"
};
static const std::unordered_set<std::string> kBoolProps = {
    "enabled","visible","looped","locked","anchored","massless","cancollide",
    "castshadow","richtext","canbedropped","requireshandle","neutral",
    "canquery","cantouch","autojumpenabled","autorotate","platformstand",
    "iswalking","isplaying","usejumppower","globalshadows","evaluatestatemachine",
    "breakjointsondeath","sit","jump","requiresneck","iscorescript",
    "allowteamchangeontouch","manualactivationonly","automaticscalingenabled",
    "archivable","robloxlocked","loaddefaultchat","defaultexitbuttonvisible",
    "canbeadded","deprecatedbehavior","requireslineofsight","screengui_enabled"
};
static const std::unordered_set<std::string> kColorProps = {
    "color3","color","backgroundcolor3","bordercolor3","textcolor3","tintcolor",
    "fogcolor","ambient","outdoorambient","lightcolor","gradienttop","gradientbottom",
    "colorshift_top","colorshift_bottom","watercolor","teamcolor","emissivetint"
};
static const std::unordered_set<std::string> kVecProps = {
    "position","size","velocity","assemblylinearvelocity","assemblyangularvelocity",
    "movedirection","cameraoffset","targetpoint","movetopoint","globalwind",
    "sunposition","moonposition","offset","gravity","airdensity","dimensions",
    "absolutesize","absoluteposition","viewportsize","viewport"
};
static const std::unordered_set<std::string> kPtrProps = {
    "parent","camerasubject","modelinstance","mouse","workspace","sky","animator",
    "humanoidrootpart","seatpart","occupant","part0","part1","attachment0","attachment1",
    "primarypart","referenceinstance","team","template","animatorinstance",
    "animation","adornee","device","currentcamera"
};
// 32-bit integers (these used to be mis-read as tiny floats and shown as "0")
static const std::unordered_set<std::string> kInt32Props = {
    "zindex","layoutorder","accountage","rigtype","cameratype","cameramode",
    "healthdisplaytype","displaydistancetype","nameocclusion","floormaterial",
    "keycode","gamepadkeycode","bodypart","shape","material","alphamode",
    "starcount","placeversion","humanoidstateid"
};
static const std::unordered_set<std::string> kInt64Props = {
    "userid","placeid","gameid","creatorid"
};

static PropRow MakeColor(PropRow r, const float v[3]) {
    float c[3];
    for (int i = 0; i < 3; ++i) c[i] = (std::max)(0.f, (std::min)(1.f, v[i]));
    int R = (int)(c[0] * 255), G = (int)(c[1] * 255), B = (int)(c[2] * 255);
    r.kind = PK_Color3; r.color = RGB(R, G, B);
    wchar_t hx[16]; swprintf(hx, 16, L"#%02X%02X%02X", R, G, B);
    r.text = hx;
    return r;
}
static std::wstring Vec3Text(const float v[3]) {
    return FmtF(v[0]) + L", " + FmtF(v[1]) + L", " + FmtF(v[2]);
}

static PropRow ReadProp(uintptr_t instAddr, const std::string& cat,
                        const std::string& name, uint64_t off) {
    PropRow r;
    r.name = W(cat + "." + name);
    r.offset = Hex(off);
    r.kind = PK_String;
    if (off == 0) { r.text = L""; return r; }
    uintptr_t a = instAddr + off;
    std::string ln = Lower(name);

    PropKind hint;
    if (LookupHint(cat, name, hint)) {
        switch (hint) {
        case PK_Vector3: { float v[3] = {0,0,0};
            if (RdBuf(a, v, 12)) { r.kind = PK_Vector3; r.text = Vec3Text(v); return r; }
            break; }
        case PK_Color3: { float v[3] = {0,0,0};
            if (RdBuf(a, v, 12)) return MakeColor(r, v);
            break; }
        case PK_Number: { float f = 0;
            if (Rd(a, f)) { r.kind = PK_Number; r.text = FmtF(f); return r; }
            break; }
        case PK_Int: { int32_t i = 0;
            if (Rd(a, i)) { r.kind = PK_Int; r.text = std::to_wstring((int)i); return r; }
            break; }
        case PK_Bool: { unsigned char b = 0;
            if (Rd(a, b)) { r.kind = PK_Bool; r.boolVal = b != 0; r.text = b ? L"true" : L"false"; return r; }
            break; }
        case PK_Pointer: { uintptr_t q = RdPtr(a); r.kind = PK_Pointer; r.text = Hex(q); return r; }
        default: break;
        }
    }
    if (kStringProps.count(ln)) {
        std::string sv;
        if (TryReadStrProp(a, sv)) { r.kind = PK_String; r.text = W(sv); return r; }
    }
    if (kBoolProps.count(ln)) {
        unsigned char b = 0; Rd(a, b);
        r.kind = PK_Bool; r.boolVal = b != 0; r.text = b ? L"true" : L"false"; return r;
    }
    if (kColorProps.count(ln)) {
        float v[3] = {0,0,0};
        if (RdBuf(a, v, 12)) return MakeColor(r, v);
    }
    if (kInt32Props.count(ln)) {
        int32_t i = 0;
        if (Rd(a, i)) { r.kind = PK_Int; r.text = std::to_wstring((int)i); return r; }
    }
    if (kInt64Props.count(ln)) {
        int64_t i = 0;
        if (Rd(a, i)) { r.kind = PK_Int; r.text = std::to_wstring((long long)i); return r; }
    }
    if (kVecProps.count(ln)) {
        float v[3] = {0,0,0};
        if (RdBuf(a, v, 12)) { r.kind = PK_Vector3; r.text = Vec3Text(v); return r; }
    }
    if (ln == "scale") {
        if (cat == "SpecialMesh" || cat == "CharacterMesh") {
            float v[3] = {0,0,0};
            if (RdBuf(a, v, 12)) { r.kind = PK_Vector3; r.text = Vec3Text(v); return r; }
        } else {
            float f = 0;
            if (Rd(a, f)) { r.kind = PK_Number; r.text = FmtF(f); return r; }
        }
    }
    if (kPtrProps.count(ln) || EndsWith(ln, "pointer")) {
        uintptr_t q = RdPtr(a);
        r.kind = PK_Pointer; r.text = Hex(q); return r;
    }
    {
        std::string sv;
        if (TryReadStrProp(a, sv) && !sv.empty() && sv.size() >= 3 && sv.size() <= 200) {
            bool hasLetter = false;
            for (char c : sv) if (isalpha((unsigned char)c)) { hasLetter = true; break; }
            if (hasLetter) { r.kind = PK_String; r.text = W(sv); return r; }
        }
    }

    // ---- generic fallback: guess from the raw 8 bytes ----
    uint64_t q = 0;
    if (!Rd(a, q)) { r.kind = PK_String; r.text = L"<bad>"; return r; }
    // 1) a heap pointer that really points to readable memory
    if (q >= 0x100000000ull && Readable((uintptr_t)q)) { r.kind = PK_Pointer; r.text = Hex(q); return r; }
    // 2) a small integer: its low dword has a zero exponent, so it can't be a sane float
    uint32_t lo = (uint32_t)(q & 0xFFFFFFFFull);
    if (lo != 0 && ((lo >> 23) & 0xFF) == 0) { r.kind = PK_Int; r.text = std::to_wstring((int)lo); return r; }
    // 3) a sane float
    float f = 0.0f; memcpy(&f, &lo, 4);
    if (f != 0.0f && !std::isnan(f) && !std::isinf(f) && std::fabs(f) < 1.0e10f) {
        r.kind = PK_Number; r.text = FmtF(f); return r;
    }
    if (IsUserPtr((uintptr_t)q)) { r.kind = PK_Pointer; r.text = Hex(q); return r; }
    r.kind = PK_Int; r.text = std::to_wstring((long long)(int64_t)q);
    return r;
}

// ============================================================
//                  building the property list
// ============================================================
static bool ComputeModelPivot(uintptr_t modelAddr, float out[3]) {
    auto kids = Children(modelAddr);
    for (uintptr_t c : kids) {
        std::string cls = ClassName(c);
        bool isPart = (cls=="Part"||cls=="MeshPart"||cls=="WedgePart"||
                       cls=="CornerWedgePart"||cls=="TrussPart"||cls=="UnionOperation"||
                       cls=="SpawnLocation"||cls=="Seat"||cls=="VehicleSeat"||cls=="BasePart");
        if (!isPart) continue;
        uint64_t primOff = 0;
        for (const char* cn : {"Part","MeshPart","WedgePart","CornerWedgePart",
                               "TrussPart","UnionOperation","SpawnLocation","Seat",
                               "VehicleSeat","BasePart"}) {
            auto it = g_classProps.find(cn);
            if (it == g_classProps.end()) continue;
            auto p = it->second.find("Primitive");
            if (p != it->second.end() && p->second) { primOff = p->second; break; }
        }
        if (!primOff) continue;
        uintptr_t prim = RdPtr(c + primOff);
        if (!IsUserPtr(prim)) continue;
        uint64_t posOff = 0;
        auto pp = g_classProps.find("Primitive");
        if (pp != g_classProps.end()) {
            auto pi = pp->second.find("Position");
            if (pi != pp->second.end()) posOff = pi->second;
        }
        if (!posOff) continue;
        if (RdBuf(prim + posOff, out, 12)) return true;
    }
    return false;
}

void BuildProps(uintptr_t addr) {
    g_allProps.clear();
    std::string cls = ClassName(addr);
    { PropRow h; h.kind = PK_Group; h.name = L"Instance"; g_allProps.push_back(h); }
    PropRow r;
    r.name = L"Name";      r.kind = PK_String;  r.text = W(InstName(addr)); g_allProps.push_back(r);
    r.name = L"ClassName"; r.kind = PK_String;  r.text = W(cls);            g_allProps.push_back(r);
    r.name = L"Address";   r.kind = PK_Pointer; r.text = Hex(addr);         g_allProps.push_back(r);
    if (HasOff("Parent")) {
        uintptr_t p = RdPtr(addr + Off("Parent"));
        r.name = L"Parent"; r.kind = PK_Pointer; r.text = Hex(p);
        if (IsUserPtr(p)) r.text += L"  (" + W(InstName(p)) + L")";
        g_allProps.push_back(r);
    }
    auto kids = Children(addr);
    r.name = L"Children"; r.kind = PK_Int;
    r.text = std::to_wstring(kids.size());
    g_allProps.push_back(r);

    if (cls == "Model" || cls == "PVInstance" || cls == "Folder" ||
        cls == "Tool" || cls == "Accessory" || cls == "Accoutrement" || cls == "WorldModel") {
        float v[3];
        if (ComputeModelPivot(addr, v)) {
            PropRow h; h.kind = PK_Group; h.name = L"Computed"; g_allProps.push_back(h);
            PropRow c; c.name = L"Computed.WorldPosition";
            c.kind = PK_Vector3;
            c.text = Vec3Text(v);
            g_allProps.push_back(c);
        }
    }
    for (const auto& cn : ClassChain(cls)) {
        if (cn == "Instance") continue;
        auto it = g_classProps.find(cn);
        if (it == g_classProps.end() || it->second.empty()) continue;
        bool anyValid = false;
        for (auto& pr : it->second) if (pr.second != 0) { anyValid = true; break; }
        if (!anyValid) continue;
        PropRow h; h.kind = PK_Group; h.name = W(cn); g_allProps.push_back(h);
        for (const auto& pr : it->second) {
            if (pr.second == 0) continue;
            g_allProps.push_back(ReadProp(addr, cn, pr.first, pr.second));
        }
    }
}

void CopyText(const std::wstring& s) {
    if (!OpenClipboard(g_main)) return;
    EmptyClipboard();
    size_t bytes = (s.size() + 1) * sizeof(wchar_t);
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (h) { memcpy(GlobalLock(h), s.c_str(), bytes); GlobalUnlock(h); SetClipboardData(CF_UNICODETEXT, h); }
    CloseClipboard();
}

// ============================================================
//                  list view population
// ============================================================
void ApplyProps(bool keepScroll) {
    if (!g_props) return;
    int top = keepScroll ? ListView_GetTopIndex(g_props) : 0;
    SendMessageW(g_props, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_props);
    for (size_t i = 0; i < g_propRows.size(); ++i) {
        auto& r = g_propRows[i];
        LVITEMW it{};
        it.mask = LVIF_TEXT;
        it.iItem = (int)i;
        it.pszText = (LPWSTR)r.name.c_str();
        ListView_InsertItem(g_props, &it);
        ListView_SetItemText(g_props, (int)i, 1, (LPWSTR)r.text.c_str());
        ListView_SetItemText(g_props, (int)i, 2, (LPWSTR)r.offset.c_str());
    }
    if (keepScroll && top > 0 && !g_propRows.empty()) {
        int per = ListView_GetCountPerPage(g_props);
        int last = (std::min)((int)g_propRows.size() - 1, top + per - 1);
        ListView_EnsureVisible(g_props, last, FALSE);
    }
    SendMessageW(g_props, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_props, nullptr, TRUE);
    g_propHot = -1;
    UpdateVisibility();
}

// Rebuild g_propRows from g_allProps: text filter + collapsed groups
void RebuildPropRows() {
    g_propRows.clear();
    wchar_t qbuf[128] = {0};
    if (g_propFilter) GetWindowTextW(g_propFilter, qbuf, 128);
    std::wstring lq = qbuf;
    for (auto& c : lq) c = towlower(c);
    bool filtering = !lq.empty();

    std::unordered_map<std::wstring, int> cnt;
    { std::wstring cg;
      for (auto& r : g_allProps) {
          if (r.kind == PK_Group) { cg = r.name; cnt[cg] = 0; }
          else cnt[cg]++;
      } }

    PropRow pending; bool pendingPushed = true, collapsed = false;
    for (auto& r : g_allProps) {
        if (r.kind == PK_Group) {
            pending = r;
            pending.count = cnt[r.name];
            collapsed = !filtering && g_collapsed.count(r.name) > 0;
            pending.collapsed = collapsed;
            pendingPushed = false;
            if (!filtering) { g_propRows.push_back(pending); pendingPushed = true; }
            continue;
        }
        if (collapsed) continue;
        if (filtering) {
            std::wstring n = r.name;
            for (auto& c : n) c = towlower(c);
            if (n.find(lq) == std::wstring::npos) continue;
        }
        if (!pendingPushed) { g_propRows.push_back(pending); pendingPushed = true; }
        g_propRows.push_back(r);
    }
}
void FilterProps() {
    RebuildPropRows();
    ApplyProps(false);
}
void ShowProps(HTREEITEM sel) {
    g_allProps.clear(); g_propRows.clear();
    g_propSub.clear();
    if (sel) {
        uintptr_t a = ItemAddr(sel);
        BuildProps(a);
        g_propSub = W(InstName(a)) + L"  (" + W(ClassName(a)) + L")";
    }
    FilterProps();
    if (g_main) InvalidateRect(g_main, nullptr, FALSE);
}
void ToggleGroup(const std::wstring& name) {
    if (g_collapsed.count(name)) g_collapsed.erase(name); else g_collapsed.insert(name);
    RebuildPropRows();
    ApplyProps(true);
}
void ToggleAllGroups() {
    bool anyOpen = false;
    for (auto& r : g_allProps) if (r.kind == PK_Group && !g_collapsed.count(r.name)) { anyOpen = true; break; }
    if (anyOpen) { for (auto& r : g_allProps) if (r.kind == PK_Group) g_collapsed.insert(r.name); }
    else g_collapsed.clear();
    RebuildPropRows();
    ApplyProps(false);
    InvalidateRect(g_main, nullptr, FALSE);
}
bool AnyGroupOpen() {
    for (auto& r : g_allProps) if (r.kind == PK_Group && !g_collapsed.count(r.name)) return true;
    return false;
}

// ============================================================
//                  custom draw
// ============================================================
static RECT PropCell(int row, int col) {
    RECT b{}; ListView_GetItemRect(g_props, row, &b, LVIR_BOUNDS);
    int x = b.left;
    for (int i = 0; i < col; ++i) x += ListView_GetColumnWidth(g_props, i);
    RECT r = b; r.left = x; r.right = x + ListView_GetColumnWidth(g_props, col);
    return r;
}

LRESULT HandlePropsCustomDraw(LPNMLVCUSTOMDRAW pcd) {
    DWORD stage = pcd->nmcd.dwDrawStage;
    if (stage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;

    if (stage == CDDS_ITEMPREPAINT) {
        int row = (int)pcd->nmcd.dwItemSpec;
        if (row < 0 || row >= (int)g_propRows.size()) return CDRF_DODEFAULT;
        auto& r = g_propRows[row];
        if (r.kind == PK_Group) {
            HDC hdc = pcd->nmcd.hdc;
            RECT rc{}; ListView_GetItemRect(g_props, row, &rc, LVIR_BOUNDS);
            int saved = SaveDC(hdc);
            IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);   // never bleed into other rows
            bool hot = (row == g_propHot);
            FillC(hdc, rc, hot ? Mix(Theme::GROUP_HDR_BG, Theme::TEXT, 0.07) : Theme::GROUP_HDR_BG);
            HLine(hdc, rc.left, rc.right, rc.top, Theme::BORDER_SOFT);
            HLine(hdc, rc.left, rc.right, rc.bottom - 1, Theme::BORDER);
            int cx = rc.left + DP(14), cy = (rc.top + rc.bottom) / 2;
            POINT tri[3];
            if (r.collapsed) { tri[0] = {cx - DP(2), cy - DP(4)}; tri[1] = {cx - DP(2), cy + DP(4)}; tri[2] = {cx + DP(3), cy}; }
            else             { tri[0] = {cx - DP(4), cy - DP(2)}; tri[1] = {cx + DP(4), cy - DP(2)}; tri[2] = {cx, cy + DP(3)}; }
            PIC_FillPoly(hdc, tri, 3, Theme::TEXT_MUTED, Theme::TEXT_MUTED);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, Theme::TEXT);
            SelectObject(hdc, g_fontBold);
            RECT t = rc; t.left += DP(26); t.right -= DP(10);
            DrawTextW(hdc, r.name.c_str(), -1, &t,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            SIZE sz{}; GetTextExtentPoint32W(hdc, r.name.c_str(), (int)r.name.size(), &sz);
            SelectObject(hdc, g_fontSmall);
            SetTextColor(hdc, Theme::TEXT_FAINT);
            wchar_t cb[16]; swprintf(cb, 16, L"%d", r.count);
            RECT ct = rc; ct.left = t.left + sz.cx + DP(8);
            if (ct.left < rc.right - DP(20))
                DrawTextW(hdc, cb, -1, &ct, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            RestoreDC(hdc, saved);
            return CDRF_SKIPDEFAULT;
        }
        pcd->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS | CDIS_HOT);
        return CDRF_NOTIFYSUBITEMDRAW;
    }

    if (stage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
        int row = (int)pcd->nmcd.dwItemSpec;
        if (row < 0 || row >= (int)g_propRows.size()) return CDRF_DODEFAULT;
        auto& r = g_propRows[row];
        int col = pcd->iSubItem;
        HDC hdc = pcd->nmcd.hdc;
        RECT rc = PropCell(row, col);
        if (rc.right <= rc.left) return CDRF_SKIPDEFAULT;

        // Everything for this cell is clipped to the cell, so text can never be
        // painted over (or under) the neighbouring column.
        int saved = SaveDC(hdc);
        IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

        bool selected = (ListView_GetItemState(g_props, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        bool hot = (row == g_propHot);
        COLORREF base = (row & 1) ? Theme::BG_ALT : Theme::BG_PANEL;
        COLORREF bg = selected ? Theme::SEL : (hot ? Theme::HOVER : base);
        if (!selected && r.changedAt) {
            DWORD age = GetTickCount() - r.changedAt;
            if (age < 900) bg = Mix(bg, Theme::ACCENT_DARK, 0.6 * (1.0 - age / 900.0));
        }
        FillC(hdc, rc, bg);
        HLine(hdc, rc.left, rc.right, rc.bottom - 1, Theme::BORDER_SOFT);
        if (col >= 1) VLine(hdc, rc.left, rc.top, rc.bottom, Theme::BORDER_SOFT);

        SetBkMode(hdc, TRANSPARENT);
        int x = rc.left + DP(10);
        std::wstring txt;
        COLORREF fg = selected ? RGB(255, 255, 255) : Theme::TEXT;
        HFONT use = g_fontBody;
        if (col == 0) {
            x = rc.left + DP(26);
            txt = r.name;
            size_t d = txt.find(L'.');
            if (d != std::wstring::npos) txt = txt.substr(d + 1);
        } else if (col == 1) {
            txt = r.text;
            if (!selected) {
                if (r.kind == PK_Number || r.kind == PK_Int || r.kind == PK_Vector3) fg = Theme::VAL_NUM;
                else if (r.kind == PK_Pointer) fg = Theme::TEXT_MUTED;
            }
            if (r.kind == PK_Pointer || r.kind == PK_Color3) use = g_fontMono;
            if (r.kind == PK_Bool || r.kind == PK_Color3) {
                int s = DP(14), by = rc.top + (rc.bottom - rc.top - s) / 2;
                RECT bx = {x, by, x + s, by + s};
                if (r.kind == PK_Bool) {
                    if (r.boolVal) {
                        PIC_FillRounded(hdc, bx, DP(4), Theme::ACCENT, Theme::ACCENT);
                        HPEN pen = CreatePen(PS_SOLID, (std::max)(1, DP(2)), RGB(255, 255, 255));
                        HGDIOBJ op = SelectObject(hdc, pen);
                        MoveToEx(hdc, x + DP(3), by + DP(7), nullptr);
                        LineTo(hdc, x + DP(6), by + DP(10));
                        LineTo(hdc, x + DP(11), by + DP(4));
                        SelectObject(hdc, op); DeleteObject(pen);
                    } else {
                        PIC_FillRounded(hdc, bx, DP(4), Theme::FIELD, Theme::BORDER);
                    }
                } else {
                    PIC_FillRounded(hdc, bx, DP(4), r.color, Theme::BORDER);
                }
                x += s + DP(8);
                if (!selected) fg = Theme::TEXT_MUTED;
            }
        } else {
            txt = r.offset;
            use = g_fontMono;
            if (!selected) fg = Theme::TEXT_FAINT;
        }
        SetTextColor(hdc, fg);
        RECT t = {x, rc.top, rc.right - DP(6), rc.bottom};
        SelectObject(hdc, use);
        DrawTextW(hdc, txt.c_str(), -1, &t,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        RestoreDC(hdc, saved);
        return CDRF_SKIPDEFAULT;
    }
    return CDRF_DODEFAULT;
}
