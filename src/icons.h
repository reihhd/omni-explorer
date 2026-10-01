// icons.h - procedural tree icons + app icon
#pragma once
#include "common.h"

void  RebuildIcons();            // (re)creates the tree image list at the current DPI
HICON LoadAppIcon(int size);     // Omni.ico next to the exe, else a generated icon
