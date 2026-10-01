#include "offsets.h"
#include "state.h"
#include "util.h"
#include "json.h"
#include "memory.h"

bool OffsetsFileChanged() {
    WIN32_FILE_ATTRIBUTE_DATA d{};
    std::wstring p = GetExeDir() + OFFSETS_REL;
    if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &d)) return false;
    return CompareFileTime(&d.ftLastWriteTime, &g_offStamp) != 0;
}

bool LoadOffsets(std::string& err) {
    std::wstring p = GetExeDir() + OFFSETS_REL;
    FILE* f = _wfopen(p.c_str(), L"rb");
    if (!f) { err = "Offsets file missing: " + Narrow(p); return false; }
    std::string txt; char buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) txt.append(buf, n);
    fclose(f);

    WIN32_FILE_ATTRIBUTE_DATA fa{};
    if (GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &fa)) g_offStamp = fa.ftLastWriteTime;

    J root; JP jp(txt);
    if (!jp.parse(root) || root.t != 3) { err = "offsets.json is invalid"; return false; }
    g_offVersion.clear();
    if (root.has("Roblox Version") && root.at("Roblox Version").t == 1) g_offVersion = root.at("Roblox Version").s;

    const J* r = nullptr;
    if (root.has("Offsets")) r = &root.at("Offsets");
    else if (root.has("offsets")) r = &root.at("offsets");
    else if (root.has("FakeDataModel") || root.has("Instance") || root.has("Player")) r = &root;
    if (!r || r->t != 3) { err = "Unrecognized offsets structure"; return false; }

    g_off.clear(); g_classProps.clear();
    auto get = [&](const char* cat, const char* fld, const char* key) {
        if (r->has(cat) && r->at(cat).has(fld)) {
            uint64_t v = 0;
            if (JNum(r->at(cat).at(fld), v)) g_off[key] = v;
        }
    };
    get("FakeDataModel", "Pointer",       "FakeDataModelPointer");
    get("FakeDataModel", "RealDataModel", "FakeDataModelToDataModel");
    get("VisualEngine",  "Pointer",       "VisualEnginePointer");
    get("VisualEngine",  "FakeDataModel", "VisualEngineToDataModel1");
    get("FakeDataModel", "RealDataModel", "VisualEngineToDataModel2");
    get("Instance", "Name",            "Name");
    get("Instance", "NameContainer",   "NameContainer");
    get("Instance", "ClassDescriptor", "ClassDescriptor");
    get("Instance", "ClassName",       "ClassDescriptorToClassName");
    get("Instance", "ChildrenStart",   "Children");
    get("Instance", "Children",        "Children");
    get("Instance", "ChildList",       "Children");
    get("Instance", "ChildrenEnd",     "ChildrenEnd");
    get("Instance", "Parent",          "Parent");
    get("Misc",     "StringLength",    "StringLength");
    if (!HasOff("StringLength")) g_off["StringLength"] = 0x10;

    for (auto& kv : r->o) {
        if (kv.second.t != 3) continue;
        const std::string& cat = kv.first;
        if (cat=="Misc"||cat=="TaskScheduler"||cat=="RenderView"||cat=="ScriptContext"||
            cat=="RenderJob"||cat=="WindowInputState"||cat=="ByteCode"||cat=="LRUHolder"||
            cat=="MemEnforcedLRUCache"||cat=="LRUNode"||cat=="CachedItem"||cat=="FileMeshData")
            continue;
        for (auto& pr : kv.second.o) {
            uint64_t v = 0;
            if (JNum(pr.second, v)) g_classProps[cat][pr.first] = v;
        }
    }
    for (const char* req : {"Name", "ClassDescriptor", "ClassDescriptorToClassName", "Children"})
        if (!HasOff(req)) { err = std::string("Missing required offset: ") + req; return false; }
    return true;
}
