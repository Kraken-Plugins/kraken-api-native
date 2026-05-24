<!-- PROJECT LOGO -->
<br />
<div align="center">
  <h3 align="center">Kraken API Native</h3>

  <p align="center">
    A native C++ foundation for launching, instrumenting, and experimenting with the Old School RuneScape Windows client.
    <br />
  </p>
</div>

[![License][license-shield]][license-url]
[![C++20][cpp-shield]][cpp-url]
[![CMake][cmake-shield]][cmake-url]

---

# Getting Started

Kraken API Native is an early-stage C++ project for working with the native
Windows Old School RuneScape client. It currently provides a launcher,
DLL injection, a debug console, memory patch helpers, and a MinHook-based log
hook foundation.

This project is for educational plugin/API development only. It is not meant
for botting, automation, account rule-breaking, malware, or bypass tooling.
Use at your own risk. The developers are not responsible for consequences
resulting from the use of this software.

## Quick Start

Clone the project and build it with a Visual Studio x64 CMake toolchain:

```shell
git clone https://github.com/cbartram/kraken-api-native
cd kraken-api-native

cmake -S . -B cmake-build-debug -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-debug --config Debug
```

When using CLion, configure the CMake profile with:

| Field | Value |
| --- | --- |
| Toolchain | Visual Studio |
| Generator | Visual Studio 17 2022 |
| Architecture | amd64 / x64 |
| Build type | Debug |

Build output is generated under the Visual Studio configuration directory:

```text
cmake-build-debug/
  launcher/
    Debug/
      launcher.exe
      plugin-core.dll
```

Run `launcher.exe` as administrator.

## Project Topology

The codebase is split into two targets:

```text
launcher.exe
  starts the OSRS client
  waits for basic GUI readiness
  injects plugin-core.dll with LoadLibraryW

plugin-core.dll
  runs inside the OSRS client process
  opens a debug console
  applies in-process patches
  installs native hooks
```

### State Ownership

- `launcher` owns the OSRS process handle, primary thread handle, DLL path, and
  injection lifecycle.
- `plugin-core` owns in-process patching, hook registration, and console
  logging after injection.
- Hard-coded client RVAs live in `plugin-core/offsets.hpp` so client-version
  dependent addresses are easy to audit.

### Feedback

- The launcher reports process launch and injection failures to its console.
- The injected DLL opens its own debug console for patch/hook status.
- A future UI shell should move this to IPC so logs can render inside the
  launcher window instead of an `AllocConsole` window.

### Timing

- The launcher uses `WaitForInputIdle` before injection instead of a fixed
  startup sleep.
- The DLL does minimal work in `DllMain`, then starts a worker thread for
  console setup, patching, and hook installation.

## Source Layout

```text
.
├── CMakeLists.txt
├── launcher/
│   ├── CMakeLists.txt
│   ├── dll_injector.cpp
│   ├── dll_injector.hpp
│   ├── main.cpp
│   ├── process_launcher.cpp
│   ├── process_launcher.hpp
│   └── win32_handle.hpp
└── plugin-core/
    ├── CMakeLists.txt
    ├── dllmain.cpp
    ├── hooks.cpp
    ├── hooks.hpp
    ├── logger.cpp
    ├── logger.hpp
    ├── offsets.hpp
    ├── patcher.cpp
    └── patcher.hpp
```

## C++ File Structure

Use `.hpp` files for declarations: class names, function signatures, shared
types, constants, and small inline utilities.

Use `.cpp` files for implementation: function bodies, private helpers, global
state, Win32 calls, MinHook calls, and logic that should compile once.

This matters because `#include` copies header text into every `.cpp` file that
includes it. If a header defines normal functions or globals, adding a second
include site can create duplicate linker symbols. This repo keeps hook state
and patch logic in `.cpp` files for that reason.

## Prerequisites

- Windows 10/11
- Visual Studio Build Tools 2022 with Desktop development with C++
- Windows 10 or Windows 11 SDK
- CMake 3.26+
- Git
- CLion or another CMake-aware IDE

The project uses C++20 and fetches [MinHook](https://github.com/TsudaKageyu/minhook)
with CMake `FetchContent`.

## Configuring The Client Path

The launcher currently uses a hard-coded default client path in:

```cpp
launcher/main.cpp
```

Update `kDefaultClientPath` if your OSRS native client is installed elsewhere:

```cpp
constexpr wchar_t kDefaultClientPath[] =
    L"C:\\Program Files (x86)\\Jagex Launcher\\Games\\Old School RuneScape\\Client\\osclient.exe";
```

The DLL path is resolved at runtime relative to `launcher.exe`, and the build
copies `plugin-core.dll` next to the executable.

## Building

From a Visual Studio developer shell:

```shell
cmake -S . -B cmake-build-debug -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-debug --config Debug
```

From CLion:

1. Open the repository.
2. Configure the toolchain as Visual Studio x64.
3. Reload CMake.
4. Build the `launcher` target.

Building `launcher` also builds `plugin-core` and copies the DLL beside the
launcher executable.

## Running

DLL injection generally requires administrator privileges.

```text
cmake-build-debug/launcher/Debug/launcher.exe
```

Right-click `launcher.exe` and select `Run as administrator`.

Expected flow:

1. `launcher.exe` starts the OSRS native client.
2. The launcher waits for the client to become input-idle.
3. The launcher injects `plugin-core.dll`.
4. `plugin-core.dll` opens a debug console.
5. The DLL applies patches and installs hooks.

## Current Native API Surface

### Launcher

- `ProcessLauncher` starts the native client and resolves the launcher
  directory.
- `DllInjector` writes the DLL path into the target process and calls
  `LoadLibraryW` through a remote thread.
- `UniqueHandle` provides RAII ownership for Win32 `HANDLE` values.

### Plugin Core

- `Patcher` changes memory protection, writes patch bytes, flushes the
  instruction cache, and restores protection.
- `InstallLogHook` installs the current native log hook through MinHook.
- `Logger` owns console initialization and thread-safe log output.
- `offsets.hpp` centralizes hard-coded RVAs.

## Known Boundaries

This is still a prototype foundation.

- RVAs are client-version dependent and must be verified after OSRS updates.
- The current log hook depends on a specific observed string layout.
- The debug console is temporary; a UI shell should receive logs through IPC.
- There is no plugin manager, config system, event bus, or embedded client
  frame yet.
- There are no automated tests yet because the core behavior depends on a live
  Windows process and injected DLL lifecycle.

## UI Roadmap

The next major phase should add a desktop launcher shell around this topology:

```text
Qt QMainWindow
  top toolbar
  central embedded OSRS HWND
  sidebar dock for plugins/configuration
  bottom dock for logs/console output
```

Qt Widgets is the recommended starting point because it provides dock widgets,
toolbars, native window hosting, and a stable path toward a RuneLite-style
desktop frame.

## Development Workflow

1. Update or add code in the narrowest target that owns the behavior.
2. Keep headers as declarations unless code is intentionally header-only.
3. Keep hard-coded offsets in `plugin-core/offsets.hpp`.
4. Rebuild with `cmake --build cmake-build-debug --config Debug`.
5. Close the OSRS client fully before reinjecting a rebuilt DLL.
6. Run `launcher.exe` as administrator and check both launcher and DLL logs.

## Troubleshooting

### The DLL console does not appear

- Confirm that `launcher.exe` was run as administrator.
- Confirm `plugin-core.dll` exists beside `launcher.exe`.
- Confirm the OSRS client path is correct.
- Check the launcher console for injection errors.

### The launcher says the client path does not exist

Update `kDefaultClientPath` in `launcher/main.cpp`.

### The client closes or crashes after injection

- The hard-coded RVAs may no longer match the installed client.
- The hook signature or target function may have changed.
- Disable the newest patch/hook and re-enable one piece at a time.

### CMake cannot find Visual Studio

Install Visual Studio Build Tools 2022 and include:

- MSVC v143 compiler toolset
- Windows 10 or Windows 11 SDK
- CMake tools for Windows
- Desktop development with C++

---

## Built With

- [C++20](https://isocpp.org/) - Core language
- [CMake](https://cmake.org/) - Build system
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) - Windows compiler/toolchain
- [Win32 API](https://learn.microsoft.com/en-us/windows/win32/) - Process, memory, and window integration
- [MinHook](https://github.com/TsudaKageyu/minhook) - Native function hooking

---

## License

This project is licensed under the [GNU General Public License 3.0](LICENSE).

[license-shield]: https://img.shields.io/badge/license-GPLv3-blue.svg?style=for-the-badge
[license-url]: LICENSE
[cpp-shield]: https://img.shields.io/badge/C%2B%2B-20-00599C.svg?style=for-the-badge
[cpp-url]: https://isocpp.org/
[cmake-shield]: https://img.shields.io/badge/CMake-3.26%2B-064F8C.svg?style=for-the-badge
[cmake-url]: https://cmake.org/
