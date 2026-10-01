#include "process.h"
#include "state.h"
#include "util.h"
#include "memory.h"
#include "offsets.h"

DWORD FindPid() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) do {
        if (_wcsicmp(pe.szExeFile, ROBLOX_EXE) == 0) { pid = pe.th32ProcessID; break; }
    } while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    return pid;
}
uintptr_t ModBase(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W me{}; me.dwSize = sizeof(me);
    uintptr_t base = 0;
    if (Module32FirstW(snap, &me)) {
        base = (uintptr_t)me.modBaseAddr;
        do {
            if (_wcsicmp(me.szModule, ROBLOX_EXE) == 0) { base = (uintptr_t)me.modBaseAddr; break; }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}
// ...\Roblox\Versions\version-02c37bc51a384b8f\RobloxPlayerBeta.exe  ->  version-02c37bc51a384b8f
static std::string VersionFromExePath(HANDLE h) {
    wchar_t buf[MAX_PATH * 2] = {0};
    DWORD sz = (DWORD)(sizeof(buf) / sizeof(buf[0]));
    if (!QueryFullProcessImageNameW(h, 0, buf, &sz)) return "";
    std::string p = Lower(Narrow(buf));
    size_t pos = p.find("version-");
    if (pos == std::string::npos) return "";
    size_t end = p.find_first_of("\\/", pos);
    return p.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}
bool Attach(std::string& err) {
    if (g_proc) { CloseHandle(g_proc); g_proc = nullptr; }
    g_classCache.clear();
    g_childrenCache.clear();
    DWORD pid = FindPid();
    if (!pid) { err = "Roblox is not running."; return false; }
    g_proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!g_proc) { err = "OpenProcess failed (run as Administrator)."; return false; }
    g_pid = pid;
    g_base = ModBase(pid);
    if (!g_base) { err = "Module base not found."; return false; }
    g_procVersion = VersionFromExePath(g_proc);
    return LoadOffsets(err);
}
uintptr_t GetDataModel() {
    if (HasOff("FakeDataModelPointer") && HasOff("FakeDataModelToDataModel")) {
        uintptr_t fk = RdPtr(g_base + Off("FakeDataModelPointer"));
        if (IsUserPtr(fk)) {
            uintptr_t dm = RdPtr(fk + Off("FakeDataModelToDataModel"));
            if (IsUserPtr(dm)) return dm;
        }
    }
    if (HasOff("VisualEnginePointer") && HasOff("VisualEngineToDataModel1") && HasOff("VisualEngineToDataModel2")) {
        uintptr_t ve = RdPtr(g_base + Off("VisualEnginePointer"));
        if (IsUserPtr(ve)) {
            uintptr_t fk = RdPtr(ve + Off("VisualEngineToDataModel1"));
            if (IsUserPtr(fk)) {
                uintptr_t dm = RdPtr(fk + Off("VisualEngineToDataModel2"));
                if (IsUserPtr(dm)) return dm;
            }
        }
    }
    return 0;
}
std::wstring VersionNote() {
    if (g_offVersion.empty() || g_procVersion.empty()) return L"";
    if (Lower(g_offVersion) == g_procVersion) return L"";
    return L"  \u00B7  Offsets mismatch (file: " + W(g_offVersion) + L", running: " + W(g_procVersion) + L")";
}
