#pragma once

#include <string_view>

namespace kraken::plugin {

void InitializeConsole();

void LogInfo(std::string_view message);

void LogWarn(std::string_view message);

void LogError(std::string_view message);

} // namespace kraken::plugin
