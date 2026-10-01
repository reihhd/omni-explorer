// live.h - Live refresh (properties + explorer sync)
#pragma once
#include "common.h"

inline constexpr int kLiveIntervalCount = 5;
extern const int kLiveIntervals[kLiveIntervalCount];   // ms

std::wstring IntervalText(int ms);      // 250 -> "250 ms", 2000 -> "2 s"
void SetLive(bool on);
void SetLiveInterval(int ms);
void LiveTick();
