#include "process_launcher.hpp"

#include <stdexcept>
#include <vector>

namespace kraken::launcher {

ClientProcess ProcessLauncher::LaunchClient(
    const std::filesystem::path& clientPath) {
    if (!std::filesystem::exists(clientPath)) {
        throw std::runtime_error("Client executable does not exist: " +
                                 clientPath.string());
    }

    STARTUPINFOW startupInfo{};
    PROCESS_INFORMATION processInfo{};
    startupInfo.cb = sizeof(startupInfo);

    const std::wstring applicationName = clientPath.wstring();
    const std::wstring workingDirectory = clientPath.parent_path().wstring();

    const BOOL created = CreateProcessW(
        applicationName.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workingDirectory.c_str(),
        &startupInfo,
        &processInfo);

    if (!created) {
        throw std::runtime_error(FormatLastError("CreateProcessW",
                                                 GetLastError()));
    }

    ClientProcess client;
    client.process = UniqueHandle(processInfo.hProcess);
    client.primaryThread = UniqueHandle(processInfo.hThread);
    client.processId = processInfo.dwProcessId;
    return client;
}

bool ProcessLauncher::WaitForInputIdle(
    HANDLE process,
    std::chrono::milliseconds timeout) {
    const DWORD result = ::WaitForInputIdle(
        process,
        static_cast<DWORD>(timeout.count()));

    return result == 0;
}

std::filesystem::path ProcessLauncher::CurrentExecutableDirectory() {
    std::vector<wchar_t> buffer(MAX_PATH);

    while (true) {
        const DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size()));

        if (length == 0) {
            throw std::runtime_error(FormatLastError("GetModuleFileNameW",
                                                     GetLastError()));
        }

        if (length < buffer.size() - 1) {
            return std::filesystem::path(buffer.data()).parent_path();
        }

        buffer.resize(buffer.size() * 2);
    }
}

std::string ProcessLauncher::FormatLastError(const char* action,
                                             DWORD errorCode) {
    return std::string(action) + " failed with Win32 error " +
           std::to_string(errorCode);
}

} // namespace kraken::launcher
