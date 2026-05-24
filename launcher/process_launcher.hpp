#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "win32_handle.hpp"

#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace kraken::launcher {

struct ClientProcess {
    UniqueHandle process;
    UniqueHandle primaryThread;
    DWORD processId = 0;
};

class ProcessLauncher {
public:
    static ClientProcess LaunchClient(const std::filesystem::path& clientPath);

    static bool WaitForInputIdle(HANDLE process,
                                 std::chrono::milliseconds timeout);

    static std::filesystem::path CurrentExecutableDirectory();

private:
    static std::string FormatLastError(const char* action, DWORD errorCode);
};

} // namespace kraken::launcher
