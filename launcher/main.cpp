#include "dll_injector.hpp"
#include "plugin_stager.hpp"
#include "process_launcher.hpp"

#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr wchar_t kDefaultClientPath[] =
    L"C:\\Program Files (x86)\\Jagex Launcher\\Games\\Old School RuneScape\\Client\\osclient.exe";

constexpr std::chrono::milliseconds kClientReadyTimeout(10000);
constexpr std::chrono::milliseconds kInjectionTimeout(10000);

std::filesystem::path ResolvePluginPath(
    const std::filesystem::path& executableDirectory) {
    const std::filesystem::path colocated =
        executableDirectory / L"plugin-core.dll";
    if (std::filesystem::exists(colocated)) {
        return colocated;
    }

    const std::filesystem::path configurationDirectory =
        executableDirectory.filename();
    const std::filesystem::path buildRoot =
        executableDirectory.parent_path().parent_path();
    const std::filesystem::path siblingTarget =
        buildRoot / L"plugin-core" / configurationDirectory /
        L"plugin-core.dll";

    if (std::filesystem::exists(siblingTarget)) {
        return siblingTarget;
    }

    return colocated;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    using kraken::launcher::DllInjector;
    using kraken::launcher::PluginStager;
    using kraken::launcher::ProcessLauncher;

    try {
        std::filesystem::path clientPath(kDefaultClientPath);
        std::filesystem::path pluginPath =
            ResolvePluginPath(ProcessLauncher::CurrentExecutableDirectory());

        for (int i = 1; i < argc; ++i) {
            const std::wstring argument(argv[i]);
            if (argument == L"--client" && i + 1 < argc) {
                clientPath = argv[++i];
            } else if (argument == L"--plugin" && i + 1 < argc) {
                pluginPath = argv[++i];
            }
        }

        std::wcout << L"[*] Client path: " << clientPath << L"\n";
        std::wcout << L"[*] Plugin source: " << pluginPath << L"\n";

        auto client = ProcessLauncher::LaunchClient(clientPath);
        std::wcout << L"[+] Client launched. PID: " << client.processId
                   << L"\n";

        if (!ProcessLauncher::WaitForInputIdle(client.process.Get(),
                                               kClientReadyTimeout)) {
            std::wcout << L"[!] Client did not report input-idle before the "
                          L"timeout. Continuing with injection.\n";
        }

        const std::filesystem::path stagedPlugin =
            PluginStager::StageForProcess(pluginPath, client.processId);
        std::wcout << L"[*] Staged plugin: " << stagedPlugin << L"\n";

        std::string injectionError;
        if (!DllInjector::Inject(client.process.Get(),
                                 client.processId,
                                 stagedPlugin,
                                 kInjectionTimeout,
                                 &injectionError)) {
            throw std::runtime_error("Injection failed: " + injectionError);
        }

        std::wcout << L"[+] DLL injected successfully.\n";
        std::wcout << L"[*] Waiting for the client process to exit.\n";
        WaitForSingleObject(client.process.Get(), INFINITE);
        std::wcout << L"[*] Client exited.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[!] " << ex.what() << "\n";
        return 1;
    }
}
