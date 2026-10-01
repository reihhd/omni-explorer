// props.h - Properties pane (data model + ListView custom draw)
#pragma once
#include "common.h"

enum PropKind { PK_String, PK_Number, PK_Int, PK_Bool, PK_Pointer, PK_Vector3, PK_Color3, PK_Group };

struct PropRow {
    PropKind kind = PK_String;
    std::wstring name, text, offset;
    bool boolVal = false;
    COLORREF color = RGB(0, 0, 0);
    bool collapsed = false;      // PK_Group only
    int  count = 0;              // PK_Group only
    DWORD changedAt = 0;         // flash timestamp (Live mode)
};

extern std::vector<PropRow> g_allProps;   // everything for the selected instance
extern std::vector<PropRow> g_propRows;   // after filter + collapsed groups

std::wstring FmtF(float f);
void BuildProps(uintptr_t addr);
void RebuildPropRows();
void ApplyProps(bool keepScroll = false);
void FilterProps();
void ShowProps(HTREEITEM sel);
void ToggleGroup(const std::wstring& name);
void ToggleAllGroups();
bool AnyGroupOpen();
void CopyText(const std::wstring& s);
LRESULT HandlePropsCustomDraw(LPNMLVCUSTOMDRAW pcd);
