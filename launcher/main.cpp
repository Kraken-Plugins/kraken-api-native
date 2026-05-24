#include "dll_injector.hpp"
#include "process_launcher.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

constexpr wchar_t kDefaultClientPath[] =
    L"C:\\Program Files (x86)\\Jagex Launcher\\Games\\Old School RuneScape\\Client\\osclient.exe";

constexpr std::chrono::milliseconds kClientReadyTimeout(10000);
constexpr std::chrono::milliseconds kInjectionTimeout(10000);

} // namespace

int main() {
    using kraken::launcher::DllInjector;
    using kraken::launcher::ProcessLauncher;

    try {
        const std::filesystem::path clientPath(kDefaultClientPath);
        const std::filesystem::path dllPath =
            ProcessLauncher::CurrentExecutableDirectory() /
            L"plugin-core.dll";

        std::cout << "[*] Launching client: " << clientPath.string() << "\n";
        auto client = ProcessLauncher::LaunchClient(clientPath);
        std::cout << "[+] Client launched. PID: " << client.processId << "\n";

        std::cout << "[*] Waiting for the client window to become idle...\n";
        if (!ProcessLauncher::WaitForInputIdle(client.process.Get(),
                                               kClientReadyTimeout)) {
            std::cout << "[!] Client did not report input-idle before the "
                         "timeout. Continuing with injection.\n";
        }

        if (!DllInjector::Inject(client.process.Get(),
                                 client.processId,
                                 dllPath,
                                 kInjectionTimeout)) {
            std::cerr << "[!] Injection failed.\n";
            return 1;
        }

        std::cout << "[*] Launcher holding. Press Enter to exit.\n";
        std::cin.get();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[!] " << ex.what() << "\n";
        return 1;
    }
}
