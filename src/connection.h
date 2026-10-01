// connection.h - attach / detach / polling
#pragma once
#include "common.h"

void Disconnect(const std::wstring& msg);
void DoRefresh();
void AutoConnect();
void PollConnection();
