// classmeta.h - class hierarchy + tree icon mapping
#pragma once
#include "common.h"

enum IconKind {
    IC_FOLDER = 0, IC_MODEL, IC_PART, IC_SCRIPT, IC_PLAYER,
    IC_CAMERA, IC_LIGHT, IC_GUI, IC_SOUND, IC_TOOL,
    IC_MESH, IC_ATTACH, IC_PARTICLE, IC_SERVICE, IC_GAME,
    IC_EVENT, IC_VALUE, IC_WORLD,
    IC_TEXT,                       // "T" icon for TextLabel / TextButton / TextBox
    IC_DEFAULT, IC_COUNT
};

std::string ParentClass(const std::string& c);
int         IconForClass(const std::string& c);
std::vector<std::string> ClassChain(const std::string& cls);
