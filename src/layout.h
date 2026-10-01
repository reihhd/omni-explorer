// layout.h - window geometry
#pragma once
#include "common.h"

struct PaneRects { RECT all{}, hdr{}, flt{}, body{}; };

inline constexpr int SIDE_ITEMS = 4;     // Explorer, Properties, Refresh, Live
inline constexpr int SPLIT_W    = 6;
enum { HB_EX_COLLAPSE = 0, HB_EX_REFRESH, HB_PR_COLLAPSE, HB_PR_LIVE, HB_COUNT };

extern RECT g_rcSide, g_rcStatus, g_rcDot, g_rcArea, g_rcSplit, g_rcConnect;
extern RECT g_rcSideItem[SIDE_ITEMS];
extern RECT g_hb[HB_COUNT];
extern PaneRects g_ex, g_pr;

void ComputeLayout(int cw, int ch);
void ComputeHeaderButtons();
void ResizePropColumns();
void UpdateVisibility();
void UpdateTips();
void Layout(int cw, int ch);
void RelayoutNow();
