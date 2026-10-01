#include "memory.h"

bool RdBuf(uintptr_t a, void* buf, size_t size) {
    if (!g_proc) return false;
    SIZE_T n = 0;
    return ReadProcessMemory(g_proc, (LPCVOID)a, buf, size, &n) && n == size;
}
uintptr_t RdPtr(uintptr_t a) { uintptr_t v = 0; Rd(a, v); return v; }
bool Readable(uintptr_t p) { unsigned char c = 0; return IsUserPtr(p) && Rd(p, c); }
uint64_t Off(const char* k) { auto it = g_off.find(k); return it == g_off.end() ? 0 : it->second; }
bool HasOff(const char* k) { return g_off.count(k) > 0; }

std::string ReadStr(uintptr_t a) {
    int32_t len = 0;
    if (!Rd(a + Off("StringLength"), len)) return "";
    if (len <= 0 || len > 1000) return "";
    uintptr_t d = (len > 15) ? RdPtr(a) : a;
    if (!d) return "";
    std::string s(len, '\0');
    if (!RdBuf(d, &s[0], len)) return "";
    size_t z = s.find('\0'); if (z != std::string::npos) s.resize(z);
    return s;
}
bool TryReadStrProp(uintptr_t a, std::string& out) {
    int32_t len = 0;
    if (!Rd(a + 0x10, len)) return false;
    if (len <= 0 || len > 2048) return false;
    uintptr_t d = (len > 15) ? RdPtr(a) : a;
    if (!IsUserPtr(d)) return false;
    std::string s(len, '\0');
    if (!RdBuf(d, &s[0], len)) return false;
    int printable = 0;
    for (unsigned char c : s) if (c >= 32 && c < 127) printable++;
    if (printable * 10 < len * 7) return false;
    char z = 0; Rd(d + len, z);
    if (z != 0) return false;
    out = s;
    return true;
}
std::string InstName(uintptr_t a) {
    if (HasOff("NameContainer")) {
        uintptr_t c = RdPtr(a + Off("NameContainer"));
        if (c) { std::string n = ReadStr(c + Off("Name")); if (!n.empty() && n != "<null>") return n; }
    }
    if (HasOff("Name")) {
        uintptr_t p = RdPtr(a + Off("Name"));
        if (p) { std::string n = ReadStr(p); if (!n.empty()) return n; }
    }
    return "<null>";
}
std::string ClassName(uintptr_t a) {
    auto it = g_classCache.find(a);
    if (it != g_classCache.end()) return it->second;
    std::string result = "<unknown>";
    if (HasOff("ClassDescriptor") && HasOff("ClassDescriptorToClassName")) {
        uintptr_t cd = RdPtr(a + Off("ClassDescriptor"));
        if (IsUserPtr(cd)) {
            uintptr_t cp = RdPtr(cd + Off("ClassDescriptorToClassName"));
            if (IsUserPtr(cp)) { std::string s = ReadStr(cp); if (!s.empty()) result = s; }
        }
    }
    g_classCache[a] = result;
    return result;
}
bool ValidClassName(const std::string& n) {
    if (n.size() < 2 || n.size() > 64) return false;
    for (unsigned char c : n) if (!(isalnum(c) || c == '_')) return false;
    return true;
}

std::vector<uintptr_t> ReadChildrenRaw(uintptr_t a) {
    std::vector<uintptr_t> out;
    if (!HasOff("Children")) return out;
    const uint64_t endOff = HasOff("ChildrenEnd") ? Off("ChildrenEnd") : 8;
    uintptr_t base = a + Off("Children");
    auto harvest = [&](uintptr_t s, uintptr_t e, std::vector<uintptr_t>& dst) -> bool {
        if (!IsUserPtr(s) || !IsUserPtr(e) || e <= s) return false;
        size_t num = (size_t)((e - s) / sizeof(uintptr_t));
        if (num == 0 || num > MAX_CHILDREN) return false;
        std::vector<uintptr_t> raw(num);
        if (!RdBuf(s, raw.data(), num * sizeof(uintptr_t))) return false;
        std::unordered_set<uintptr_t> seen;
        int valid = 0;
        for (uintptr_t c : raw) {
            if (IsUserPtr(c) && !seen.count(c) && ValidClassName(ClassName(c))) {
                seen.insert(c); dst.push_back(c); ++valid;
            }
        }
        if (!raw.empty() && valid * 4 < (int)raw.size()) { dst.clear(); return false; }
        return !dst.empty();
    };
    uintptr_t s = RdPtr(base), e = RdPtr(base + endOff);
    if (harvest(s, e, out)) return out;
    uintptr_t lp = RdPtr(base);
    if (IsUserPtr(lp)) { s = RdPtr(lp); e = RdPtr(lp + endOff); if (harvest(s, e, out)) return out; }
    return out;
}
std::vector<uintptr_t> Children(uintptr_t a) {
    auto it = g_childrenCache.find(a);
    if (it != g_childrenCache.end()) return it->second;
    std::vector<uintptr_t> kids = ReadChildrenRaw(a);
    g_childrenCache[a] = kids;
    return kids;
}
std::vector<uintptr_t> ChildrenStable(uintptr_t a) {
    auto it = g_childrenCache.find(a);
    if (it != g_childrenCache.end() && !it->second.empty()) return it->second;
    const int delays[] = {0, 20, 40, 80};
    for (int i = 0; i < 4; ++i) {
        if (delays[i]) Sleep(delays[i]);
        g_childrenCache.erase(a);
        std::vector<uintptr_t> kids = ReadChildrenRaw(a);
        if (!kids.empty()) { g_childrenCache[a] = kids; return kids; }
    }
    g_childrenCache[a] = {};
    return {};
}
bool HasChildrenCached(uintptr_t a) {
    auto it = g_childrenCache.find(a);
    if (it != g_childrenCache.end()) return !it->second.empty();
    std::vector<uintptr_t> kids = ReadChildrenRaw(a);
    bool has = !kids.empty();
    g_childrenCache[a] = std::move(kids);
    return has;
}
