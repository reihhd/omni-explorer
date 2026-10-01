// paint.h - everything drawn directly on the main window
#pragma once
#include "common.h"
#include "layout.h"

void SetStatus(const std::wstring& s);
void Toast(const std::wstring& what);
RECT ClearRect(const PaneRects& p);
void PaintMain(HWND hwnd);
