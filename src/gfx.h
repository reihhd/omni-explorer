// gfx.h - small GDI drawing helpers
#pragma once
#include "common.h"

void     FillC(HDC dc, const RECT& r, COLORREF c);
void     HLine(HDC dc, int x1, int x2, int y, COLORREF c);
void     VLine(HDC dc, int x, int y1, int y2, COLORREF c);
COLORREF Mix(COLORREF a, COLORREF b, double t);
COLORREF PIC_Darken(COLORREF c, int d);
COLORREF PIC_Lighten(COLORREF c, int d);
void PIC_FillPoly(HDC hdc, POINT* pts, int n, COLORREF fill, COLORREF border);
void PIC_FillCircle(HDC hdc, int cx, int cy, int r, COLORREF fill, COLORREF border);
void PIC_FillRounded(HDC hdc, RECT r, int rad, COLORREF fill, COLORREF border);
void DrawRing(HDC dc, int cx, int cy, int r, int w, COLORREF c);
