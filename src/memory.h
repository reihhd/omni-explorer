// memory.h - reading the target process + Instance helpers
#pragma once
#include "common.h"
#include "state.h"

template <class T> inline bool Rd(uintptr_t a, T& out) {
    if (!g_proc) return false;
    SIZE_T n = 0;
    return ReadProcessMemory(g_proc, (LPCVOID)a, &out, sizeof(T), &n) && n == sizeof(T);
}
bool      RdBuf(uintptr_t a, void* buf, size_t size);
uintptr_t RdPtr(uintptr_t a);
inline bool IsUserPtr(uintptr_t p) { return p >= 0x10000 && p <= 0x00007FFFFFFFFFFFull; }
bool      Readable(uintptr_t p);
uint64_t  Off(const char* k);
bool      HasOff(const char* k);

std::string ReadStr(uintptr_t a);
bool        TryReadStrProp(uintptr_t a, std::string& out);
std::string InstName(uintptr_t a);
std::string ClassName(uintptr_t a);
bool        ValidClassName(const std::string& n);

std::vector<uintptr_t> ReadChildrenRaw(uintptr_t a);   // always reads memory, no cache
std::vector<uintptr_t> Children(uintptr_t a);          // cached
std::vector<uintptr_t> ChildrenStable(uintptr_t a);    // cached + retries (used when expanding)
bool                   HasChildrenCached(uintptr_t a);
