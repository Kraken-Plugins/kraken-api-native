#include "logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <cstdio>
#include <iostream>
#include <mutex>

namespace kraken::plugin {
namespace {

std::mutex g_logMutex;

void LogLine(std::ostream& stream,
             std::string_view level,
             std::string_view message) {
    std::lock_guard lock(g_logMutex);
    stream << "[" << level << "] " << message << "\n";
}

} // namespace

void InitializeConsole() {
    static bool initialized = false;
    if (initialized) {
        return;
    }

    AllocConsole();

    FILE* output = nullptr;
    FILE* error = nullptr;
    freopen_s(&output, "CONOUT$", "w", stdout);
    freopen_s(&error, "CONOUT$", "w", stderr);

    initialized = true;
}

void LogInfo(std::string_view message) {
    LogLine(std::cout, "INFO", message);
}

void LogWarn(std::string_view message) {
    LogLine(std::cout, "WARN", message);
}

void LogError(std::string_view message) {
    LogLine(std::cerr, "ERROR", message);
}

} // namespace kraken::plugin
