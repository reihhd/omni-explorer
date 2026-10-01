#include "connection.h"
#include "state.h"
#include "util.h"
#include "memory.h"
#include "process.h"
#include "offsets.h"
#include "tree.h"
#include "props.h"
#include "layout.h"
#include "paint.h"

void Disconnect(const std::wstring& msg) {
    if (g_proc) { CloseHandle(g_proc); g_proc = nullptr; }
    g_connected = false; g_dm = 0; g_lastErr = msg;
    g_filterActive = false;
    if (g_tree) TreeView_DeleteAllItems(g_tree);
    g_populated.clear(); g_childrenCache.clear(); g_classCache.clear();
    g_allProps.clear(); g_propRows.clear(); g_propSub.clear();
    ApplyProps(false);
    SetWindowTextW(g_main, L"Omni");
    SetStatus(msg);
    UpdateVisibility();
    InvalidateRect(g_main, nullptr, FALSE);
}
void DoRefresh() {
    std::string err;
    if (!Attach(err)) { Disconnect(W(err)); return; }
    g_lastErr.clear();
    LoadRoot(true);
}
void AutoConnect() {
    std::string err;
    if (!Attach(err)) {
        std::wstring e = W(err);
        if (e != g_lastErr) { g_lastErr = e; SetStatus(e); if (g_main) InvalidateRect(g_main, nullptr, FALSE); }
        return;
    }
    LoadRoot(false);
}
void PollConnection() {
    if (g_connected) {
        DWORD code = 0;
        if (!g_proc || !GetExitCodeProcess(g_proc, &code) || code != STILL_ACTIVE) {
            Disconnect(L"Roblox closed \u2014 waiting for a new process\u2026");
            return;
        }
        // offsets.json edited while running -> reload it automatically
        if (OffsetsFileChanged()) {
            std::string err;
            if (LoadOffsets(err)) { LoadRoot(true); SetStatus(L"offsets.json reloaded"); }
            else SetStatus(W(err));
            return;
        }
        static uintptr_t pending = 0;
        uintptr_t dm = GetDataModel();
        if (IsUserPtr(dm) && dm != g_dm) {
            if (pending == dm) { pending = 0; LoadRoot(false); }   // new DataModel (e.g. teleport)
            else pending = dm;
        } else pending = 0;
    } else {
        if (FindPid()) AutoConnect();
    }
}
