// offsets.h - offsets.json loader
#pragma once
#include "common.h"

bool LoadOffsets(std::string& err);
bool OffsetsFileChanged();     // true when offsets.json was modified after the last load
