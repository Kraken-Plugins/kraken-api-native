#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "logger.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace kraken::plugin {

class IpcServer {
public:
    IpcServer() = default;
    ~IpcServer();

    IpcServer(const IpcServer&) = delete;
    IpcServer& operator=(const IpcServer&) = delete;

    bool Start(std::wstring pipeName);

    void Stop();

private:
    void Run(std::wstring pipeName);

    void HandleLine(std::string_view line);

    void SendLog(const LogRecord& record);

    void SendResult(std::string_view id,
                    bool ok,
                    std::string_view message);

    bool SendLine(std::string_view line);

    HANDLE GetPipeHandle();

    void SetPipeHandle(HANDLE pipe);

    std::atomic_bool running_{false};
    std::thread worker_;
    std::mutex pipeMutex_;
    std::mutex writeMutex_;
    HANDLE pipe_ = INVALID_HANDLE_VALUE;
};

std::wstring ResolveConfiguredPipeName();

} // namespace kraken::plugin
