#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace kraken::plugin {

enum class LogLevel {
    Info,
    Warn,
    Error,
};

struct LogRecord {
    LogLevel level;
    std::string message;
};

using LogSink = std::function<void(const LogRecord&)>;

void InitializeLogging();

void InitializeDebugConsole();

void SetLogSink(LogSink sink);

std::vector<LogRecord> SnapshotBufferedLogs();

std::string_view LogLevelName(LogLevel level);

void LogInfo(std::string_view message);

void LogWarn(std::string_view message);

void LogError(std::string_view message);

} // namespace kraken::plugin
