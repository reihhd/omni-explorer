#include "gfx.h"

void FillC(HDC dc, const RECT& r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c); FillRect(dc, &r, b); DeleteObject(b);
}
void HLine(HDC dc, int x1, int x2, int y, COLORREF c) { RECT r = {x1, y, x2, y + 1}; FillC(dc, r, c); }
void VLine(HDC dc, int x, int y1, int y2, COLORREF c) { RECT r = {x, y1, x + 1, y2}; FillC(dc, r, c); }
COLORREF Mix(COLORREF a, COLORREF b, double t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return RGB((int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t),
               (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t),
               (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t));
}
COLORREF PIC_Darken(COLORREF c, int d) {
    return RGB((std::max)(0, (int)GetRValue(c) - d),
               (std::max)(0, (int)GetGValue(c) - d),
               (std::max)(0, (int)GetBValue(c) - d));
}
COLORREF PIC_Lighten(COLORREF c, int d) {
    return RGB((std::min)(255, (int)GetRValue(c) + d),
               (std::min)(255, (int)GetGValue(c) + d),
               (std::min)(255, (int)GetBValue(c) + d));
}
void PIC_FillPoly(HDC hdc, POINT* pts, int n, COLORREF fill, COLORREF border) {
    HBRUSH b = CreateSolidBrush(fill); HPEN p = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(hdc, b); HGDIOBJ op = SelectObject(hdc, p);
    Polygon(hdc, pts, n);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(b); DeleteObject(p);
}
void PIC_FillCircle(HDC hdc, int cx, int cy, int r, COLORREF fill, COLORREF border) {
    HBRUSH b = CreateSolidBrush(fill); HPEN p = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(hdc, b); HGDIOBJ op = SelectObject(hdc, p);
    Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(b); DeleteObject(p);
}
void PIC_FillRounded(HDC hdc, RECT r, int rad, COLORREF fill, COLORREF border) {
    HBRUSH b = CreateSolidBrush(fill); HPEN p = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(hdc, b); HGDIOBJ op = SelectObject(hdc, p);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(b); DeleteObject(p);
}
void DrawRing(HDC dc, int cx, int cy, int r, int w, COLORREF c) {
    HPEN pen = CreatePen(PS_SOLID, (std::max)(1, w), c);
    HGDIOBJ op = SelectObject(dc, pen);
    HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
}
