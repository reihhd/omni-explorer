#include "util.h"

int g_dpi = 96;

std::wstring W(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}
std::string Narrow(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
std::wstring Hex(uint64_t v) {
    wchar_t b[32]; swprintf(b, 32, L"0x%llX", (unsigned long long)v); return b;
}
std::string Lower(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}
std::wstring GetExeDir() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    size_t slash = dir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) dir = dir.substr(0, slash + 1);
    return dir;
}
HFONT MakeFont(int px, int weight, const wchar_t* face) {
    LOGFONTW lf{};
    lf.lfHeight = -DP(px);
    lf.lfWeight = weight;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpynW(lf.lfFaceName, face ? face : L"Segoe UI Variable Text", LF_FACESIZE);
    HFONT f = CreateFontIndirectW(&lf);
    if (!f) {
        lstrcpynW(lf.lfFaceName, face ? face : L"Segoe UI", LF_FACESIZE);
        f = CreateFontIndirectW(&lf);
    }
    return f;
}
