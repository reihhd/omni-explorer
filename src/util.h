// util.h - strings, fonts, DPI
#pragma once
#include "common.h"

extern int g_dpi;
inline int DP(int px) { return MulDiv(px, g_dpi, 96); }

std::wstring W(const std::string& s);
std::string  Narrow(const std::wstring& w);
std::wstring Hex(uint64_t v);
std::string  Lower(std::string s);
std::wstring GetExeDir();
HFONT        MakeFont(int px, int weight, const wchar_t* face = nullptr);
