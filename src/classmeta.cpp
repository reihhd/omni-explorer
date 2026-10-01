#include "classmeta.h"

std::string ParentClass(const std::string& c) {
    static const std::unordered_map<std::string, std::string> m = {
        {"Part","BasePart"},{"WedgePart","BasePart"},{"CornerWedgePart","BasePart"},
        {"TrussPart","BasePart"},{"MeshPart","BasePart"},{"UnionOperation","BasePart"},
        {"SpawnLocation","BasePart"},{"Seat","BasePart"},{"VehicleSeat","BasePart"},
        {"BasePart","Primitive"},{"Primitive","Instance"},
        {"Model","PVInstance"},{"PVInstance","Instance"},
        {"Player","Instance"},{"Humanoid","Instance"},{"Camera","Instance"},
        {"Tool","Instance"},{"Lighting","Instance"},{"Workspace","Instance"},
        {"Sky","Instance"},{"Atmosphere","Instance"},{"Sound","Instance"},
        {"Attachment","Instance"},{"Weld","Instance"},{"WeldConstraint","Instance"},
        {"SpecialMesh","Instance"},{"CharacterMesh","Instance"},{"Decal","Instance"},
        {"Texture","Decal"},{"Clothing","Instance"},
        {"GuiObject","GuiBase2D"},{"GuiBase2D","Instance"},
        {"TextLabel","GuiObject"},{"TextButton","GuiObject"},
        {"ImageLabel","GuiObject"},{"ImageButton","GuiObject"},
        {"Frame","GuiObject"},{"ScrollingFrame","GuiObject"},{"TextBox","GuiObject"},
        {"ScreenGui","LayerCollector"},{"SurfaceGui","LayerCollector"},
        {"BillboardGui","LayerCollector"},{"LayerCollector","GuiBase2D"},
        {"ProximityPrompt","Instance"},{"ClickDetector","Instance"},
        {"DragDetector","Instance"},{"AnimationTrack","Instance"},
        {"Animator","Instance"},{"PlayerMouse","Instance"},
        {"BloomEffect","PostEffect"},{"BlurEffect","PostEffect"},
        {"SunRaysEffect","PostEffect"},{"ColorCorrectionEffect","PostEffect"},
        {"DepthOfFieldEffect","PostEffect"},{"ColorGradingEffect","PostEffect"},
        {"PostEffect","Instance"},
        {"ParticleEmitter","Instance"},{"Beam","Instance"},
        {"Script","BaseScript"},{"LocalScript","BaseScript"},{"ModuleScript","BaseScript"},
        {"BaseScript","Instance"},
        {"Accessory","Accoutrement"},{"Accoutrement","Instance"},{"Hat","Accoutrement"},
    };
    auto it = m.find(c);
    return it == m.end() ? "" : it->second;
}

int IconForClass(const std::string& c) {
    static const std::unordered_map<std::string, int> m = {
        {"Folder", IC_FOLDER},
        {"Model", IC_MODEL}, {"PVInstance", IC_MODEL}, {"WorldModel", IC_MODEL},
        {"Part", IC_PART}, {"WedgePart", IC_PART}, {"CornerWedgePart", IC_PART},
        {"TrussPart", IC_PART}, {"MeshPart", IC_PART}, {"UnionOperation", IC_PART},
        {"SpawnLocation", IC_PART}, {"Seat", IC_PART}, {"VehicleSeat", IC_PART},
        {"Script", IC_SCRIPT}, {"LocalScript", IC_SCRIPT}, {"ModuleScript", IC_SCRIPT},
        {"Player", IC_PLAYER}, {"Players", IC_PLAYER}, {"Humanoid", IC_PLAYER}, {"Animator", IC_PLAYER},
        {"AnimationTrack", IC_PLAYER}, {"Camera", IC_CAMERA},
        {"Lighting", IC_LIGHT}, {"Sky", IC_LIGHT}, {"Atmosphere", IC_LIGHT},
        {"BloomEffect", IC_LIGHT}, {"BlurEffect", IC_LIGHT}, {"SunRaysEffect", IC_LIGHT},
        {"ColorCorrectionEffect", IC_LIGHT}, {"DepthOfFieldEffect", IC_LIGHT},
        {"ColorGradingEffect", IC_LIGHT},
        // text-based GUI objects get the "T" icon
        {"TextLabel", IC_TEXT}, {"TextButton", IC_TEXT}, {"TextBox", IC_TEXT},
        {"ScreenGui", IC_GUI}, {"GuiObject", IC_GUI}, {"Frame", IC_GUI},
        {"ImageLabel", IC_GUI}, {"ImageButton", IC_GUI}, {"ScrollingFrame", IC_GUI},
        {"SurfaceGui", IC_GUI}, {"BillboardGui", IC_GUI}, {"GuiBase2D", IC_GUI},
        {"Sound", IC_SOUND},
        {"Tool", IC_TOOL}, {"HopperBin", IC_TOOL}, {"Accessory", IC_TOOL},
        {"SpecialMesh", IC_MESH}, {"CharacterMesh", IC_MESH}, {"MeshContentProvider", IC_MESH},
        {"Attachment", IC_ATTACH}, {"Weld", IC_ATTACH}, {"WeldConstraint", IC_ATTACH},
        {"Motor6D", IC_ATTACH},
        {"ParticleEmitter", IC_PARTICLE}, {"Beam", IC_PARTICLE}, {"Trail", IC_PARTICLE},
        {"Smoke", IC_PARTICLE}, {"Fire", IC_PARTICLE}, {"Sparkles", IC_PARTICLE},
        {"RemoteEvent", IC_EVENT}, {"RemoteFunction", IC_EVENT}, {"BindableEvent", IC_EVENT},
        {"BindableFunction", IC_EVENT}, {"UnreliableRemoteEvent", IC_EVENT},
        {"Workspace", IC_WORLD}, {"Terrain", IC_WORLD},
        {"ReplicatedStorage", IC_SERVICE}, {"ReplicatedFirst", IC_SERVICE},
        {"ServerStorage", IC_SERVICE}, {"StarterGui", IC_SERVICE}, {"StarterPack", IC_SERVICE},
        {"StarterPlayer", IC_SERVICE}, {"StarterPlayerScripts", IC_SERVICE},
        {"StarterCharacterScripts", IC_SERVICE}, {"Teams", IC_SERVICE}, {"Chat", IC_SERVICE},
        {"Stats", IC_SERVICE}, {"Debris", IC_SERVICE},
    };
    auto it = m.find(c);
    if (it != m.end()) return it->second;
    if (c.size() > 7 && c.compare(c.size() - 7, 7, "Service") == 0) return IC_SERVICE;
    if (c.size() > 5 && c.compare(c.size() - 5, 5, "Value") == 0) return IC_VALUE;
    if (c == "DataModel" || c == "game") return IC_GAME;
    return IC_DEFAULT;
}

std::vector<std::string> ClassChain(const std::string& cls) {
    std::vector<std::string> chain;
    std::unordered_set<std::string> guard;
    std::string cur = cls;
    while (!cur.empty() && guard.insert(cur).second && chain.size() < 16) {
        chain.push_back(cur);
        cur = ParentClass(cur);
    }
    return chain;
}
