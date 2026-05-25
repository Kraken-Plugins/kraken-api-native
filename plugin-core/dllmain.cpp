#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "hooks.hpp"
#include "ipc_server.hpp"
#include "logger.hpp"
#include "offsets.hpp"
#include "patcher.hpp"

#include <string>

namespace {

// Process-lifetime server; avoid joining worker threads during DLL detach.
kraken::plugin::IpcServer* g_ipcServer = nullptr;

bool IsDebugConsoleRequested() {
    wchar_t value[8]{};
    const DWORD length = GetEnvironmentVariableW(
        L"KRAKEN_DEBUG_CONSOLE",
        value,
        static_cast<DWORD>(sizeof(value) / sizeof(value[0])));

    return length > 0 && value[0] == L'1';
}

bool ApplyInitialPatches() {
    const std::uintptr_t patchAddress =
        kraken::plugin::Patcher::FromRva(
            kraken::plugin::offsets::kInitialNopPatchRva);

    return kraken::plugin::Patcher::Nop(patchAddress, 2);
}

void PluginMain() {
    kraken::plugin::InitializeLogging();
    if (IsDebugConsoleRequested()) {
        kraken::plugin::InitializeDebugConsole();
    }

    kraken::plugin::LogInfo("==============================");
    kraken::plugin::LogInfo("plugin-core loaded.");
    kraken::plugin::LogInfo("DLL injection is working.");
    kraken::plugin::LogInfo("==============================");

    const std::wstring pipeName = kraken::plugin::ResolveConfiguredPipeName();
    if (!pipeName.empty()) {
        g_ipcServer = new kraken::plugin::IpcServer();
        if (!g_ipcServer->Start(pipeName)) {
            kraken::plugin::LogWarn("IPC server was already running.");
        }
    } else {
        kraken::plugin::LogWarn(
            "KRAKEN_PIPE_NAME was not set; UI IPC is disabled.");
    }

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
