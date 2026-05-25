#include "logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <cstdio>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace kraken::plugin {
namespace {

constexpr std::size_t kMaxBufferedLogRecords = 500;

std::mutex g_logMutex;
std::deque<LogRecord> g_bufferedLogs;
LogSink g_logSink;
bool g_loggingInitialized = false;
bool g_debugConsoleInitialized = false;

void LogLine(LogLevel level, std::string_view message) {
    LogSink sink;
    LogRecord record{level, std::string(message)};

    {
        std::lock_guard lock(g_logMutex);

        if (g_bufferedLogs.size() >= kMaxBufferedLogRecords) {
            g_bufferedLogs.pop_front();
        }

        g_bufferedLogs.push_back(record);
        sink = g_logSink;

        if (g_debugConsoleInitialized) {
            std::ostream& stream =
                level == LogLevel::Error ? std::cerr : std::cout;
            stream << "[" << LogLevelName(level) << "] "
                   << record.message << "\n";
        }
    }

    if (sink) {
        sink(record);
    }
}

} // namespace

void InitializeLogging() {
    std::lock_guard lock(g_logMutex);
    if (g_loggingInitialized) {
        return;
    }

    g_loggingInitialized = true;
}

void InitializeDebugConsole() {
    std::lock_guard lock(g_logMutex);
    if (g_debugConsoleInitialized) {
        return;
    }

    AllocConsole();

    FILE* output = nullptr;
    FILE* error = nullptr;
    freopen_s(&output, "CONOUT$", "w", stdout);
    freopen_s(&error, "CONOUT$", "w", stderr);

    g_debugConsoleInitialized = true;
}

void SetLogSink(LogSink sink) {
    LogSink replaySink;
    std::vector<LogRecord> replayLogs;

    {
        std::lock_guard lock(g_logMutex);
        g_logSink = std::move(sink);
        replaySink = g_logSink;

        if (replaySink) {
            replayLogs.assign(g_bufferedLogs.begin(), g_bufferedLogs.end());
        }
    }

    if (!replaySink) {
        return;
    }

    for (const LogRecord& record : replayLogs) {
        replaySink(record);
    }
}

std::vector<LogRecord> SnapshotBufferedLogs() {
    std::lock_guard lock(g_logMutex);
    return {g_bufferedLogs.begin(), g_bufferedLogs.end()};
}

std::string_view LogLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "?";
    }
}

void LogInfo(std::string_view message) {
    LogLine(LogLevel::Info, message);
}

void LogWarn(std::string_view message) {
    LogLine(LogLevel::Warn, message);
}

void LogError(std::string_view message) {
    LogLine(LogLevel::Error, message);
}

} // namespace kraken::plugin
