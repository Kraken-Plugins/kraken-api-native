# Kraken API Native
An Old School RuneScape API for working with the Native Windows C++ client
Provides DLL injection, a debug console, and a foundation for reading game state and rendering overlays.

> ⚠️ **This project is intended for plugin development in the spirit of RuneLite — Jagex-compliant, non-malicious tooling only.**

---

## Requirements

Before cloning or building anything, install the following:

### 1. Visual Studio Build Tools 2022
CLion needs Microsoft's compiler and Windows SDK to build this project. You do **not** need the full Visual Studio IDE.

1. Go to https://visualstudio.microsoft.com/downloads/
2. Scroll to **Tools for Visual Studio** → download **Build Tools for Visual Studio 2022**
3. Run the installer and select **Desktop development with C++**
4. Confirm the following are checked on the right-hand sidebar:
    - MSVC v143 compiler toolset
    - Windows 11 SDK (or Windows 10 SDK)
    - CMake tools for Windows
5. Install (~5 GB)
---

## Configuring

These steps only need to be done once after installing your IDE (CLion).

### Set Up the Toolchain

1. Open CLion → **File → Settings** (`Ctrl+Alt+S`)
2. Navigate to **Build, Execution, Deployment → Toolchains**
3. Click **+** → select **Visual Studio**
4. Set **Architecture** to `amd64` — this is critical, OSRS is a 64-bit process
5. Wait for all fields to show green checkmarks
6. Click **Apply**

### Set Up the CMake Profile

1. Still in Settings → **Build, Execution, Deployment → CMake**
2. Select the default profile (usually named "Debug")
3. Configure it as follows:

| Field      | Value                 |
|------------|-----------------------|
| Build type | Debug                 |
| Toolchain  | Visual Studio         |
| Generator  | Visual Studio 17 2022 |

4. Click **Apply → OK**

### Open the Project

**File → Open** → select the cloned `osrs-loader` folder.  
CLion will detect the `CMakeLists.txt` and prompt:

> "CMake project needs to be reloaded"

Click **Reload**. You should see both `launcher` and `plugin-core` appear in the project panel on the left.

---

## Configuring Your Client Path

Open `launcher/main.cpp` and update the path to your OSRS client executable:

```cpp
const std::string clientPath = "C:\\Path\\To\\RuneScape.exe";
```

The DLL path is handled automatically — it is resolved relative to `launcher.exe` at runtime, so no changes are needed there.

---

## Building

Press `Ctrl+F9` or click the **hammer icon** in the toolbar.

A successful build prints something like this in the Build panel at the bottom:

```
[1/4] Building CXX object launcher/...
[2/4] Linking CXX executable launcher.exe
[3/4] Building CXX object plugin-core/...
[4/4] Linking CXX shared library plugin-core.dll
Build finished
```

Build output is placed in:
```
cmake-build-debug/
    launcher/
        launcher.exe
        plugin-core.dll    ← copied here automatically by the build
```

---

## Running

DLL injection requires administrator privileges.

**Run from Explorer:**  
Navigate to `cmake-build-debug/launcher/`, right-click `launcher.exe` → **Run as administrator**

---

## Development Workflow

Each time you make a change to `plugin-core`:

1. Edit `plugin-core/dllmain.cpp`
2. Press `Ctrl+F9` to rebuild
3. Close the OSRS client fully (you cannot re-inject a new DLL into a running process)
4. Run `launcher.exe` again as administrator
5. Check the debug console for your output

---

## Troubleshooting

**The console window doesn't appear**
- Make sure you ran the launcher as administrator
- Check that the `clientPath` in `main.cpp` is correct
- Try increasing the `Sleep(3000)` delay in `main.cpp` if your machine is slow to start the client

**"Access is denied" error from the launcher**
- Not running as administrator — see the Running section above

**OSRS closes immediately**
- The `clientPath` is likely wrong or pointing to the launcher/wrapper instead of the actual `.exe`

**CLion doesn't show `launcher` or `plugin-core` as build targets**
- Open the CMake panel (bottom toolbar) → click the reload/refresh button

---
