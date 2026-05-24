#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "hooks.hpp"
#include "logger.hpp"
#include "offsets.hpp"
#include "patcher.hpp"

namespace {

bool ApplyInitialPatches() {
    const std::uintptr_t patchAddress =
        kraken::plugin::Patcher::FromRva(
            kraken::plugin::offsets::kInitialNopPatchRva);

    return kraken::plugin::Patcher::Nop(patchAddress, 2);
}

void PluginMain() {
    kraken::plugin::InitializeConsole();

    kraken::plugin::LogInfo("==============================");
    kraken::plugin::LogInfo("plugin-core loaded.");
    kraken::plugin::LogInfo("DLL injection is working.");
    kraken::plugin::LogInfo("==============================");

    kraken::plugin::LogInfo("Applying patches...");
    if (!ApplyInitialPatches()) {
        kraken::plugin::LogError("Initial patch failed.");
        return;
    }

    if (!kraken::plugin::InstallLogHook()) {
        kraken::plugin::LogError("Log hook installation failed.");
        return;
    }

    kraken::plugin::LogInfo("Patches applied.");
}

DWORD WINAPI PluginThread(LPVOID) {
    PluginMain();
    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);

        HANDLE thread = CreateThread(
            nullptr,
            0,
            PluginThread,
            nullptr,
            0,
            nullptr);

        if (thread != nullptr) {
            CloseHandle(thread);
        }
    }

    return TRUE;
}
