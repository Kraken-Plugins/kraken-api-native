#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace kraken::launcher {

class DllInjector {
public:
    static bool Inject(HANDLE process,
                       DWORD processId,
                       const std::filesystem::path& dllPath,
                       std::chrono::milliseconds timeout,
                       std::string* errorMessage = nullptr);
};

} // namespace kraken::launcher
