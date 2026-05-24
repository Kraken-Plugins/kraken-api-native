<!-- PROJECT LOGO -->
<br />
<div align="center">
  <h3 align="center">Kraken API Native</h3>

  <p align="center">
    A native C++ launcher and injected plugin core for experimenting with the Old School RuneScape Windows client.
    <br />
  </p>
</div>

[![License][license-shield]][license-url]
[![C++20][cpp-shield]][cpp-url]
[![CMake][cmake-shield]][cmake-url]

---

# Getting Started

Kraken API Native is an early-stage C++ project for launching the native Old
School RuneScape Windows client, injecting a DLL, and applying native patches
and hooks from inside the client process.

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
cmake --build cmake-build-debug --target launcher --config Debug
```

Run the launcher as administrator:

```text
cmake-build-debug\launcher\Debug\launcher.exe
```

DLL injection generally requires administrator privileges.

## Project Topology

The codebase is intentionally small again. There are two runtime targets:

```text
launcher.exe
  starts the OSRS client
  waits briefly for the client to become input-idle
  resolves plugin-core.dll from the build output
  stages a per-run temporary DLL copy
  injects the staged DLL with LoadLibraryW
  waits for the client process to exit

plugin-core.dll
  runs inside the OSRS client process
  opens a debug console
  applies in-process patches
  installs native hooks
```

### State Ownership

- `launcher` owns the OSRS process handle, primary thread handle, plugin DLL
  path resolution, staging path, and injection lifecycle.
- `plugin-core` owns in-process patching, hook registration, and console
  logging after injection.
- Hard-coded client RVAs live in `plugin-core/offsets.hpp` so client-version
  dependent addresses are easy to audit.

### Feedback

- `launcher.exe` writes launch, staging, and injection status to its console.
- `plugin-core.dll` opens its own debug console after injection and writes
  patch/hook diagnostics there.

### Timing

- The launcher uses `WaitForInputIdle` before injection instead of a fixed
  startup sleep.
- The launcher stages `plugin-core.dll` to a unique temp-file path before
  injection. This keeps the build output DLL from being locked by the running
  OSRS process.
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
│   ├── plugin_stager.cpp
│   ├── plugin_stager.hpp
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
- Visual Studio 2022 or Build Tools 2022
- Desktop development with C++
- Windows 10 or Windows 11 SDK
- CMake 3.26+
- Git
- CLion, Visual Studio, or another CMake-aware IDE

The native project uses C++20 and fetches
[MinHook](https://github.com/TsudaKageyu/minhook) with CMake `FetchContent`.

## Configuring The Client Path

The launcher currently uses a default client path in `launcher/main.cpp`:

```cpp
constexpr wchar_t kDefaultClientPath[] =
    L"C:\\Program Files (x86)\\Jagex Launcher\\Games\\Old School RuneScape\\Client\\osclient.exe";
```

For manual testing, override it with:

```shell
launcher.exe --client "C:\path\to\osclient.exe"
```

You can also override the plugin DLL path:

```shell
launcher.exe --plugin "C:\path\to\plugin-core.dll"
```

## Building

From a Visual Studio developer shell:

```shell
cmake -S . -B cmake-build-debug -G "Visual Studio 17 2022" -A x64
cmake --build cmake-build-debug --target launcher --config Debug
```

From CLion:

1. Open the repository.
2. Configure the toolchain as Visual Studio x64.
3. Reload CMake.
4. Build the `launcher` target.

Building `launcher` also builds `plugin-core`. The launcher resolves
`plugin-core.dll` from either:

```text
cmake-build-debug\launcher\Debug\plugin-core.dll
cmake-build-debug\plugin-core\Debug\plugin-core.dll
```

The second path is the normal CLion/Visual Studio CMake output after this
backtrack.

## Running

Run `launcher.exe` as administrator.

Expected flow:

1. `launcher.exe` starts the OSRS native client.
2. The launcher waits for the client to become input-idle.
3. The launcher copies `plugin-core.dll` to a unique temp path.
4. The launcher injects the staged DLL with `LoadLibraryW`.
5. `plugin-core.dll` opens a debug console.
6. The DLL applies patches and installs hooks.
7. The launcher process waits until the client exits.

## Current Native API Surface

### Launcher

- `ProcessLauncher` starts the native client and resolves the launcher
  directory.
- `DllInjector` writes the DLL path into the target process and calls
  `LoadLibraryW` through a remote thread.
- `PluginStager` copies the built DLL to a unique temp path before injection.
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
- There is no UI shell, plugin manager, config system, or event bus.
- There are no automated tests yet because the core behavior depends on a live
  Windows process and injected DLL lifecycle.

## Development Workflow

1. Update or add code in the narrowest target that owns the behavior.
2. Keep launcher-side process and injection logic in `launcher`.
3. Keep injected patching and hooks in `plugin-core`.
4. Keep headers as declarations unless code is intentionally header-only.
5. Keep hard-coded offsets in `plugin-core/offsets.hpp`.
6. Rebuild with `cmake --build cmake-build-debug --target launcher --config Debug`.
7. Run `launcher.exe` as administrator and check both launcher and DLL console
   logs.

## Troubleshooting

### The DLL console does not appear

- Confirm that `launcher.exe` was run as administrator.
- Confirm the OSRS client path is correct.
- Confirm `plugin-core.dll` exists under the CMake `plugin-core` output
  directory for the same configuration.
- Check the launcher console for injection errors.

### Rebuild fails because plugin-core.dll is locked

New launcher runs inject a staged temp DLL instead of the build output DLL. If
you still see a locked build-output DLL, close any OSRS process that was
launched before the staging change and rebuild.

### The launcher says the client path does not exist

Update `kDefaultClientPath` in `launcher/main.cpp`, or run the launcher with
`--client`.

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

- [C++20](https://isocpp.org/) - Native launcher and injected plugin core
- [CMake](https://cmake.org/) - Build system
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) - Windows compiler/toolchain
- [Win32 API](https://learn.microsoft.com/en-us/windows/win32/) - Process and memory integration
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
