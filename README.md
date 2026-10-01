# Omni

**Omni** is a lightweight, read-only Instance Explorer for Roblox, with a dark, Dex-style interface. It attaches to a running Roblox client, walks the `DataModel` in memory, and shows the instance tree and the properties of the selected instance, all in a native Win32 app with no external dependencies.

> Omni only **reads** memory. It never writes to, injects into, or modifies the Roblox process.

---

## Features

- **Auto-connect**: waits for `RobloxPlayerBeta.exe`, attaches automatically, and re-attaches when Roblox restarts or the `DataModel` changes (e.g. a teleport).
- **Explorer tree**: full instance hierarchy with per-class icons, lazy child loading, and tree state (expanded nodes and selection) preserved across refreshes.
- **Search**: filter the Explorer by name (`Ctrl+F`).
- **Properties pane**: grouped properties for the selected instance (strings, numbers, integers, booleans, pointers, `Vector3`, `Color3`), resolved through the class hierarchy, with its own filter box and collapsible groups. The offset of each property is shown when the pane is wide enough.
- **Live refresh**: re-reads the selected instance on a timer (250 ms to 5 s) and flashes the values that changed. Optionally keeps the Explorer tree in sync by adding, removing, and renaming nodes in expanded branches.
- **Context menus**: copy name, class name, address, or full path of an instance; copy value, name, offset, or `Name = Value` of a property; expand, collapse, or reload a node.
- **Hot-reloadable offsets**: edit `_of/offsets.json` while Omni is running and it reloads automatically. A warning appears in the status bar if the offsets' Roblox version does not match the running client.
- **Modern native UI**: custom-drawn with GDI, double-buffered, Per-Monitor DPI v2 aware, with a draggable splitter, a compact sidebar mode for narrow windows, and a stacked layout for very narrow ones.

## Keyboard shortcuts

| Key      | Action                          |
|----------|---------------------------------|
| `F5`     | Refresh / re-attach             |
| `Ctrl+F` | Focus the Explorer search box   |
| `Ctrl+L` | Toggle live refresh             |

Right-click the **Live** button (sidebar or Properties header) for interval and tree-sync options.

---

## Requirements

- Windows 10 or later (x64)
- Roblox player (`RobloxPlayerBeta.exe`) running
- **Run Omni as Administrator**, or `OpenProcess` will fail
- An up-to-date `offsets.json` matching your Roblox version (see below)
- To build: **MinGW-w64 (x86_64)** with `g++` supporting C++17, or **CMake 3.15+** with a C++17 toolchain

## Building

### Option 1: `build.bat` (MinGW-w64)

1. Install MinGW-w64 (x86_64), for example from [winlibs.com](https://winlibs.com) or MSYS2 `mingw64`, and add its `bin` folder to `PATH`.
2. Open a new terminal in the project folder and run:

   ```bat
   build.bat
   ```

3. On success, `OmniExp.exe` is created next to the script. The full compiler output is saved to `build.log`.

### Option 2: CMake

```bat
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

CMake globs `src/*.cpp`, so new source files are picked up automatically (re-run the configure step). `build.bat` lists its source files explicitly, so add new files there.

Linked libraries: `comctl32`, `uxtheme`, `dwmapi`, `gdi32`, `user32`, `kernel32`.

---

## Setup

Place the files next to `OmniExp.exe` like this:

```
OmniExp.exe
Omni.ico            (optional, a generated icon is used if missing)
_of/
└─ offsets.json     (your offsets file)
```

Then:

1. Start Roblox.
2. Run `OmniExp.exe` as Administrator.
3. Omni connects on its own; the status dot in the bottom-left turns green when attached.

### About `offsets.json`

Omni reads its memory offsets from `_of/offsets.json`. Roblox updates frequently and offsets change with almost every update, so keep this file current. The file produced by a dumper such as [RbxDumperV2](https://offsets.imtheo.lol) is supported out of the box.

The loader accepts the dumper's layout (`"Offsets": { ... }`), a lowercase `"offsets"` key, or a flat object. The following are required:

| Category / field                   | Used for                              |
|------------------------------------|---------------------------------------|
| `Instance.Name` / `NameContainer`  | Reading instance names                |
| `Instance.ClassDescriptor`         | Locating the class descriptor         |
| `Instance.ClassName`               | Reading the class name                |
| `Instance.ChildrenStart` / `ChildrenEnd` | Enumerating children            |
| `FakeDataModel.Pointer` + `RealDataModel` | Locating the `DataModel` (fallback: `VisualEngine.Pointer` + `FakeDataModel`) |

Every other category in the file (for example `BasePart`, `Humanoid`, `Lighting`) is exposed as properties for instances of the matching class. If `"Roblox Version"` is present, Omni compares it with the version folder of the running client and warns on a mismatch.

---

## Project structure

```
Omni/
├─ build.bat / CMakeLists.txt
├─ _of/offsets.json          <- put your offsets file here
└─ src/
   ├─ main.cpp               WinMain, message loop, hotkeys
   ├─ window.cpp/.h          WndProc, edit/listview subclassing, hit-testing
   ├─ state.cpp/.h           all global state (extern)
   ├─ common.h, theme.h      shared includes, color palette
   ├─ util / gfx             strings, fonts, DPI, GDI helpers
   ├─ json / offsets         JSON parser, offsets.json loader
   ├─ process / memory       attach to Roblox, read memory, Children/Name/Class
   ├─ classmeta / icons      class hierarchy, tree icons
   ├─ tree.cpp/.h            tree population, search, custom draw, SyncTree
   ├─ props.cpp/.h           property building, value types, ListView drawing
   ├─ live.cpp/.h            live refresh
   ├─ layout / paint         geometry and all UI drawing
   ├─ menus                  context menus
   └─ connection             connect, disconnect, polling
```

## How it works

1. **Find and attach**: `process` locates `RobloxPlayerBeta.exe` via a Toolhelp snapshot, opens it with `PROCESS_VM_READ | PROCESS_QUERY_INFORMATION`, and finds the module base.
2. **Locate the DataModel**: the base address plus the `FakeDataModel` pointer offset leads to the real `DataModel`, with a `VisualEngine` path as fallback.
3. **Walk the tree**: `memory` reads each instance's name, class name, and child list, validating pointers and class names and caching results. Children are loaded lazily as nodes are expanded.
4. **Show properties**: `props` combines the instance's class chain (from `classmeta`) with the offsets from `offsets.json` and formats each value by type.
5. **Stay current**: a timer polls the connection (process exit, offsets file change, new `DataModel`) and drives live refresh.

---

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| "OpenProcess failed (run as Administrator)." | Start Omni with administrator rights. |
| "Roblox is not running." | Start the Roblox player; Omni connects automatically. |
| "Offsets file missing" | Create `_of/offsets.json` next to the exe. |
| "Missing required offset: ..." | Your offsets file lacks one of the required fields listed above. |
| Empty tree or garbage names | Offsets are outdated; update `offsets.json` (watch for the "Offsets mismatch" warning). |
| `g++ was not found in PATH` | Install MinGW-w64 (x86_64) and open a **new** terminal. |

## Disclaimer

Omni reads the memory of another process. Use it only on software and in environments where that is permitted, and check Roblox's Terms of Use before running it against a live client. It is provided as-is, without warranty, and offsets may break at any time when Roblox updates.
