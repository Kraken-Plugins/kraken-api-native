#pragma once

#include <Windows.h>

#include <filesystem>

namespace kraken::launcher {

class PluginStager {
public:
    static std::filesystem::path StageForProcess(
        const std::filesystem::path& sourceDll,
        DWORD processId);
};

} // namespace kraken::launcher
