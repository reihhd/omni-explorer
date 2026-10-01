// tree.h - Explorer tree (population, state, search, live sync, custom draw)
#pragma once
#include "common.h"

uintptr_t    ItemAddr(HTREEITEM h);
std::wstring Label(uintptr_t a);
HTREEITEM    InsertNode(HTREEITEM parent, uintptr_t addr, const std::wstring& text, bool kids);
void         PopulateChildren(HTREEITEM item, uintptr_t addr);

std::vector<std::string> GetNodePath(HTREEITEM item);
std::wstring PathString(HTREEITEM item);
void SaveTreeState();
void RestoreTreeState();
void LoadRoot(bool preserveState);
void ReloadNode(HTREEITEM item);
void CollapseAllTree();

void ApplySearch();

// Live sync: adds/removes/renames nodes in expanded branches without rebuilding the tree
void SyncNode(HTREEITEM item);
void SyncTree();

LRESULT HandleTreeCustomDraw(LPNMTVCUSTOMDRAW cd);
