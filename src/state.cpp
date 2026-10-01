#include "state.h"

HANDLE    g_proc = nullptr;
DWORD     g_pid  = 0;
uintptr_t g_base = 0;
uintptr_t g_dm   = 0;
std::string g_offVersion;
std::string g_procVersion;
FILETIME  g_offStamp{};

std::map<std::string, uint64_t> g_off;
std::map<std::string, std::map<std::string, uint64_t>> g_classProps;
std::unordered_map<uintptr_t, std::string> g_classCache;
std::unordered_set<HTREEITEM> g_populated;
std::unordered_map<uintptr_t, std::vector<uintptr_t>> g_childrenCache;

HWND g_main = nullptr, g_tree = nullptr, g_props = nullptr;
HWND g_edit = nullptr, g_propFilter = nullptr, g_tip = nullptr;
HIMAGELIST g_icons = nullptr, g_rowImg = nullptr;
HFONT g_fontBody = nullptr, g_fontBold = nullptr, g_fontSmall = nullptr;
HFONT g_fontBrand = nullptr, g_fontIcon = nullptr, g_fontMono = nullptr;
HBRUSH g_brField = nullptr;

std::vector<std::vector<std::string>> g_savedExpanded;
std::vector<std::string>              g_savedSelected;
bool g_filterActive = false;
bool g_connected = false;
std::wstring g_lastErr;
std::wstring g_propSub;
std::wstring g_statusText = L"Waiting\u2026";
std::unordered_set<std::wstring> g_collapsed;

bool   g_showExplorer = true, g_showProps = true;
bool   g_stacked = false, g_dragSplit = false, g_compact = false;
bool   g_live = true, g_autoTree = true, g_trackLeave = false;
int    g_liveInterval = 1000;
double g_split = 0.55;
int    g_sideHot = -1, g_hotHb = -1, g_hotClear = -1, g_propHot = -1;
bool   g_hotConnect = false;
DWORD  g_flashUntil = 0;
