#include "plugin_stager.hpp"

#include <chrono>
#include <stdexcept>
#include <string>

namespace kraken::launcher {

std::filesystem::path PluginStager::StageForProcess(
    const std::filesystem::path& sourceDll,
    DWORD processId) {
    if (!std::filesystem::exists(sourceDll)) {
        throw std::runtime_error("Plugin DLL does not exist: " +
                                 sourceDll.string());
    }

    const std::filesystem::path stageDirectory =
        std::filesystem::temp_directory_path() / "kraken-api-native";
    std::filesystem::create_directories(stageDirectory);

    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const auto ticks =
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    const std::filesystem::path stagedDll =
        stageDirectory /
        ("plugin-core-" + std::to_string(processId) + "-" +
         std::to_string(ticks) + ".dll");

    std::filesystem::copy_file(sourceDll,
                               stagedDll,
                               std::filesystem::copy_options::overwrite_existing);

    return stagedDll;
}

} // namespace kraken::launcher
