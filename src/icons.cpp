#include "icons.h"
#include "state.h"
#include "util.h"
#include "gfx.h"
#include "theme.h"
#include "classmeta.h"

static const COLORREF ICON_KEY = RGB(255, 0, 255);   // transparent key colour

// Every icon is drawn on a 16x16 grid and scaled to the DPI-correct size.
static void PIC_DrawIcon(HDC hdc, int kind, int sz) {
    RECT full = {0, 0, sz, sz};
    HBRUSH wb = CreateSolidBrush(ICON_KEY);
    FillRect(hdc, &full, wb); DeleteObject(wb);
    SetGraphicsMode(hdc, GM_ADVANCED);
    XFORM xf{}; xf.eM11 = xf.eM22 = (float)sz / 16.0f; xf.eM12 = xf.eM21 = 0; xf.eDx = xf.eDy = 0;
    SetWorldTransform(hdc, &xf);
    switch (kind) {
    case IC_FOLDER: { COLORREF y = RGB(250, 204, 21), yd = PIC_Darken(y, 60);
        POINT tab[] = {{2,4},{6,4},{7,6},{14,6},{14,13},{2,13}};
        PIC_FillPoly(hdc, tab, 6, y, yd); break; }
    case IC_MODEL: { COLORREF c = RGB( 34, 197,  94), cd = PIC_Darken(c, 70), cl = PIC_Lighten(c, 50);
        POINT t[] = {{8,2},{14,5},{8,8},{2,5}}; POINT l[] = {{2,5},{8,8},{8,14},{2,11}};
        POINT r[] = {{8,8},{14,5},{14,11},{8,14}};
        PIC_FillPoly(hdc, t, 4, cl, cd); PIC_FillPoly(hdc, l, 4, c, cd); PIC_FillPoly(hdc, r, 4, cd, cd); break; }
    case IC_PART: { COLORREF c = RGB(249, 115,  22), cd = PIC_Darken(c, 70), cl = PIC_Lighten(c, 50);
        POINT t[] = {{8,3},{13,5},{8,7},{3,5}}; POINT l[] = {{3,5},{8,7},{8,13},{3,11}};
        POINT r[] = {{8,7},{13,5},{13,11},{8,13}};
        PIC_FillPoly(hdc, t, 4, cl, cd); PIC_FillPoly(hdc, l, 4, c, cd); PIC_FillPoly(hdc, r, 4, cd, cd); break; }
    case IC_SCRIPT: { COLORREF b = RGB( 59, 130, 246), bd = PIC_Darken(b, 60);
        POINT doc[] = {{3,2},{11,2},{13,4},{13,14},{3,14}};
        PIC_FillPoly(hdc, doc, 5, b, bd);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
        HGDIOBJ op = SelectObject(hdc, pen);
        for (int y = 6; y <= 11; y += 2) { MoveToEx(hdc, 5, y, nullptr); LineTo(hdc, 11, y); }
        SelectObject(hdc, op); DeleteObject(pen); break; }
    case IC_PLAYER: { COLORREF r = RGB(239, 68, 68), rd = PIC_Darken(r, 60);
        PIC_FillCircle(hdc, 8, 5, 3, r, rd);
        POINT body[] = {{3,14},{13,14},{11,9},{5,9}};
        PIC_FillPoly(hdc, body, 4, r, rd); break; }
    case IC_CAMERA: { COLORREF g = RGB(107, 114, 128), gd = PIC_Darken(g, 60);
        RECT body = {2,5,14,12};
        PIC_FillRounded(hdc, body, 2, g, gd);
        PIC_FillCircle(hdc, 8, 8, 3, RGB(55, 65, 81), gd);
        PIC_FillCircle(hdc, 8, 8, 1, RGB(243, 244, 246), gd); break; }
    case IC_LIGHT: { COLORREF y = RGB(250, 204, 21), yd = PIC_Darken(y, 80);
        PIC_FillCircle(hdc, 8, 8, 3, y, yd);
        HPEN pen = CreatePen(PS_SOLID, 1, yd);
        HGDIOBJ op = SelectObject(hdc, pen);
        for (int a = 0; a < 360; a += 45) {
            double rad = a * 3.14159 / 180.0;
            int x1 = 8 + (int)(4 * cos(rad)), y1 = 8 + (int)(4 * sin(rad));
            int x2 = 8 + (int)(6 * cos(rad)), y2 = 8 + (int)(6 * sin(rad));
            MoveToEx(hdc, x1, y1, nullptr); LineTo(hdc, x2, y2);
        }
        SelectObject(hdc, op); DeleteObject(pen); break; }
    case IC_GUI: { COLORREF p = RGB(168, 85, 247), pd = PIC_Darken(p, 60);
        RECT outer = {2,3,14,13};
        PIC_FillRounded(hdc, outer, 2, p, pd);
        RECT inner = {4,5,10,9};
        HBRUSH wb2 = CreateSolidBrush(RGB(255,255,255));
        FillRect(hdc, &inner, wb2); DeleteObject(wb2); break; }
    case IC_SOUND: { COLORREF p = RGB(236, 72, 153), pd = PIC_Darken(p, 60);
        POINT spk[] = {{3,6},{6,6},{9,3},{9,13},{6,10},{3,10}};
        PIC_FillPoly(hdc, spk, 6, p, pd);
        HPEN pen = CreatePen(PS_SOLID, 1, pd);
        HGDIOBJ op = SelectObject(hdc, pen);
        Arc(hdc, 7, 5, 14, 11, 12, 5, 12, 11);
        SelectObject(hdc, op); DeleteObject(pen); break; }
    case IC_TOOL: { COLORREF g = RGB(148, 163, 184), gd = PIC_Darken(g, 60);
        POINT wr[] = {{3,12},{11,4},{13,6},{5,14}};
        PIC_FillPoly(hdc, wr, 4, g, gd);
        PIC_FillCircle(hdc, 11, 5, 3, g, gd);
        PIC_FillCircle(hdc, 11, 5, 1, RGB(255,255,255), gd); break; }
    case IC_MESH: { COLORREF c = RGB( 14, 165, 233), cd = PIC_Darken(c, 70);
        POINT t[] = {{8,2},{14,5},{8,8},{2,5}}; POINT l[] = {{2,5},{8,8},{8,14},{2,11}};
        POINT r[] = {{8,8},{14,5},{14,11},{8,14}};
        PIC_FillPoly(hdc, t, 4, PIC_Lighten(c, 50), cd); PIC_FillPoly(hdc, l, 4, c, cd); PIC_FillPoly(hdc, r, 4, cd, cd);
        HPEN pen = CreatePen(PS_SOLID, 1, cd);
        HGDIOBJ op = SelectObject(hdc, pen);
        MoveToEx(hdc, 8, 2, nullptr); LineTo(hdc, 8, 8); MoveToEx(hdc, 8, 8, nullptr); LineTo(hdc, 8, 14);
        MoveToEx(hdc, 2, 5, nullptr); LineTo(hdc, 14, 5); MoveToEx(hdc, 2, 11, nullptr); LineTo(hdc, 14, 11);
        SelectObject(hdc, op); DeleteObject(pen); break; }
    case IC_ATTACH: { COLORREF s = RGB(148, 163, 184), sd = PIC_Darken(s, 60);
        PIC_FillCircle(hdc, 8, 8, 4, s, sd);
        PIC_FillCircle(hdc, 8, 8, 2, RGB(255,255,255), sd); break; }
    case IC_PARTICLE: { COLORREF c = RGB( 20, 184, 166), cd = PIC_Darken(c, 60);
        PIC_FillCircle(hdc, 5, 5, 2, c, cd); PIC_FillCircle(hdc, 11, 6, 2, c, cd);
        PIC_FillCircle(hdc, 8, 11, 2, c, cd); PIC_FillCircle(hdc, 12, 12, 1, c, cd); break; }
    case IC_SERVICE: { COLORREF b = Theme::ACCENT, bd = Theme::ACCENT_DARK;
        for (int a = 0; a < 360; a += 45) {
            double rad = a * 3.14159 / 180.0;
            int x = 8 + (int)(5 * cos(rad)), y = 8 + (int)(5 * sin(rad));
            RECT r = {x-1, y-1, x+2, y+2};
            HBRUSH br = CreateSolidBrush(bd);
            FillRect(hdc, &r, br); DeleteObject(br);
        }
        PIC_FillCircle(hdc, 8, 8, 4, b, bd);
        PIC_FillCircle(hdc, 8, 8, 2, RGB(255,255,255), bd); break; }
    case IC_GAME: { COLORREF g = RGB( 34, 197,  94), gd = PIC_Darken(g, 60);
        PIC_FillCircle(hdc, 8, 8, 6, g, gd);
        POINT tri[] = {{6,4},{6,12},{12,8}};
        PIC_FillPoly(hdc, tri, 3, RGB(255,255,255), RGB(255,255,255)); break; }
    case IC_EVENT: { COLORREF y = RGB(250, 176, 5), yd = PIC_Darken(y, 80);
        POINT bolt[] = {{9,1},{4,9},{7,9},{6,15},{12,6},{8,6}};
        PIC_FillPoly(hdc, bolt, 6, y, yd); break; }
    case IC_VALUE: { COLORREF c = RGB(6, 182, 212), cd = PIC_Darken(c, 70);
        RECT r = {2, 3, 14, 13};
        PIC_FillRounded(hdc, r, 3, c, cd);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
        HGDIOBJ op = SelectObject(hdc, pen);
        MoveToEx(hdc, 5, 7, nullptr); LineTo(hdc, 11, 7);
        MoveToEx(hdc, 5, 10, nullptr); LineTo(hdc, 11, 10);
        SelectObject(hdc, op); DeleteObject(pen); break; }
    case IC_WORLD: { COLORREF b = RGB(59, 130, 246), bd = PIC_Darken(b, 70), g = RGB(74, 222, 128);
        PIC_FillCircle(hdc, 8, 8, 6, b, bd);
        POINT l1[] = {{5,4},{8,3},{9,6},{7,8},{4,7}};
        POINT l2[] = {{9,9},{12,8},{12,11},{9,12}};
        PIC_FillPoly(hdc, l1, 5, g, g); PIC_FillPoly(hdc, l2, 4, g, g); break; }
    case IC_TEXT: { // indigo rounded square with a white "T"
        COLORREF c = RGB(99, 102, 241), cd = PIC_Darken(c, 70);
        RECT box = {2, 2, 14, 14};
        PIC_FillRounded(hdc, box, 3, c, cd);
        POINT t[] = {{4,4},{12,4},{12,6},{9,6},{9,12},{7,12},{7,6},{4,6}};
        PIC_FillPoly(hdc, t, 8, RGB(255,255,255), RGB(255,255,255)); break; }
    default: { COLORREF s = RGB(148, 163, 184), sd = PIC_Darken(s, 60);
        RECT r = {3, 3, 13, 13};
        PIC_FillRounded(hdc, r, 3, s, sd); break; }
    }
    ModifyWorldTransform(hdc, nullptr, MWT_IDENTITY);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
}

void RebuildIcons() {
    int sz = DP(16);
    HIMAGELIST nl = ImageList_Create(sz, sz, ILC_COLOR32 | ILC_MASK, IC_COUNT, 1);
    HDC hdcScreen = GetDC(nullptr);
    for (int i = 0; i < IC_COUNT; ++i) {
        HDC hdc = CreateCompatibleDC(hdcScreen);
        HBITMAP bmp = CreateCompatibleBitmap(hdcScreen, sz, sz);
        HBITMAP ob = (HBITMAP)SelectObject(hdc, bmp);
        PIC_DrawIcon(hdc, i, sz);
        SelectObject(hdc, ob);
        DeleteDC(hdc);
        ImageList_AddMasked(nl, bmp, ICON_KEY);
        DeleteObject(bmp);
    }
    ReleaseDC(nullptr, hdcScreen);
    if (g_tree) {
        HIMAGELIST old = TreeView_SetImageList(g_tree, nl, TVSIL_NORMAL);
        if (old) ImageList_Destroy(old);
    }
    g_icons = nl;
}

// ---- application icon ----
static HICON CreateProceduralIcon(int size) {
    HDC hdcScreen = GetDC(nullptr);
    HDC hdc = CreateCompatibleDC(hdcScreen);
    HBITMAP bmpColor = CreateCompatibleBitmap(hdcScreen, size, size);
    HBITMAP oldC = (HBITMAP)SelectObject(hdc, bmpColor);
    RECT r = {0,0,size,size};
    HBRUSH white = CreateSolidBrush(RGB(255,255,255));
    FillRect(hdc, &r, white); DeleteObject(white);
    int pad = size / 8;
    RECT body = {pad, pad, size - pad, size - pad};
    HBRUSH bg = CreateSolidBrush(Theme::ACCENT);
    HPEN np = CreatePen(PS_NULL, 0, RGB(0,0,0));
    HGDIOBJ ob = SelectObject(hdc, bg); HGDIOBJ op = SelectObject(hdc, np);
    RoundRect(hdc, body.left, body.top, body.right, body.bottom, size/4, size/4);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(bg); DeleteObject(np);
    DrawRing(hdc, size / 2, size / 2, size / 3, (std::max)(2, size / 8), RGB(255,255,255));
    SelectObject(hdc, oldC);
    DeleteDC(hdc); ReleaseDC(nullptr, hdcScreen);

    HBITMAP bmpMask = CreateBitmap(size, size, 1, 1, nullptr);
    HDC hdcMask = CreateCompatibleDC(nullptr);
    HBITMAP oldM = (HBITMAP)SelectObject(hdcMask, bmpMask);
    RECT r2 = {0,0,size,size};
    HBRUSH black = CreateSolidBrush(RGB(0,0,0));
    FillRect(hdcMask, &r2, black); DeleteObject(black);
    SelectObject(hdcMask, oldM);
    DeleteDC(hdcMask);
    ICONINFO ii{};
    ii.fIcon = TRUE; ii.hbmColor = bmpColor; ii.hbmMask = bmpMask;
    HICON h = CreateIconIndirect(&ii);
    DeleteObject(bmpColor); DeleteObject(bmpMask);
    return h;
}
HICON LoadAppIcon(int size) {
    std::wstring p = GetExeDir() + APP_ICON;
    HICON h = (HICON)LoadImageW(nullptr, p.c_str(), IMAGE_ICON, size, size, LR_LOADFROMFILE);
    if (!h) h = (HICON)LoadImageW(nullptr, p.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    if (!h) h = CreateProceduralIcon(size);
    return h;
}
