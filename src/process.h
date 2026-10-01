// process.h - find / attach to Roblox, locate the DataModel
#pragma once
#include "common.h"

DWORD        FindPid();
uintptr_t    ModBase(DWORD pid);
bool         Attach(std::string& err);
uintptr_t    GetDataModel();
std::wstring VersionNote();     // "" when versions match / unknown, otherwise a warning suffix
