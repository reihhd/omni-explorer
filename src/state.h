// state.h - global application state (defined in state.cpp)
#pragma once
#include "common.h"

// ---- config ----
inline constexpr const wchar_t* ROBLOX_EXE   = L"RobloxPlayerBeta.exe";
inline constexpr const wchar_t* OFFSETS_REL  = L"_of\\offsets.json";
inline constexpr const wchar_t* APP_ICON     = L"Omni.ico";
inline constexpr size_t MAX_CHILDREN   = 20000;
inline constexpr size_t MAX_SCAN_NODES = 300000;

// ---- process / memory ----
extern HANDLE    g_proc;
extern DWORD     g_pid;
extern uintptr_t g_base;
extern uintptr_t g_dm;
extern std::string g_offVersion;     // "Roblox Version" from offsets.json
extern std::string g_procVersion;    // version-xxxx parsed from the running exe path
extern FILETIME  g_offStamp;         // last-write time of offsets.json when loaded

extern std::map<std::string, uint64_t> g_off;
extern std::map<std::string, std::map<std::string, uint64_t>> g_classProps;
extern std::unordered_map<uintptr_t, std::string> g_classCache;
extern std::unordered_set<HTREEITEM> g_populated;
extern std::unordered_map<uintptr_t, std::vector<uintptr_t>> g_childrenCache;

// ---- windows / gdi ----
extern HWND g_main, g_tree, g_props, g_edit, g_propFilter, g_tip;
extern HIMAGELIST g_icons, g_rowImg;
extern HFONT g_fontBody, g_fontBold, g_fontSmall, g_fontBrand, g_fontIcon, g_fontMono;
extern HBRUSH g_brField;

// ---- tree state ----
extern std::vector<std::vector<std::string>> g_savedExpanded;
extern std::vector<std::string>              g_savedSelected;
extern bool g_filterActive;
extern bool g_connected;
extern std::wstring g_lastErr;
extern std::wstring g_propSub;
extern std::wstring g_statusText;
extern std::unordered_set<std::wstring> g_collapsed;

// ---- UI state ----
extern bool   g_showExplorer, g_showProps;
extern bool   g_stacked, g_dragSplit, g_compact;
extern bool   g_live, g_autoTree, g_trackLeave;
extern int    g_liveInterval;            // ms
extern double g_split;
extern int    g_sideHot, g_hotHb, g_hotClear, g_propHot;
extern bool   g_hotConnect;
extern DWORD  g_flashUntil;

// ---- ids ----
enum { ID_TREE = 100, ID_PROPS, ID_EDIT, ID_REFRESH, ID_PROPFILTER };
enum { IDM_NAME = 500, IDM_CLASS, IDM_ADDR, IDM_PATH, IDM_EXPAND, IDM_COLLAPSE, IDM_RELOAD,
       IDM_VAL, IDM_PNAME, IDM_POFF, IDM_PPAIR,
       IDM_LIVE_TOGGLE = 600, IDM_TREE_SYNC, IDM_INT_BASE = 610 };
inline constexpr UINT_PTR TIMER_DEBOUNCE = 1;
inline constexpr UINT_PTR TIMER_ANIM     = 2;
inline constexpr UINT_PTR TIMER_LIVE     = 3;
inline constexpr UINT_PTR TIMER_POLL     = 4;
