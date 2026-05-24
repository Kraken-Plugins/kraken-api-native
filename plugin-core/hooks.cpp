#include "hooks.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "logger.hpp"
#include "offsets.hpp"
#include "patcher.hpp"

#include <MinHook.h>
#include <Windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>

namespace kraken::plugin {
namespace {

constexpr std::size_t kMaxLogMessageLength = 4096;

using LogFunction = void(__fastcall*)(void*, std::int32_t, void*);

LogFunction g_originalLog = nullptr;

std::string_view LogLevelName(std::int32_t level) {
    switch (level) {
        case 0:
            return "TRACE";
        case 1:
            return "DEBUG";
        case 2:
            return "INFO";
        case 3:
            return "WARN";
        case 4:
            return "ERROR";
        case 5:
            return "FATAL";
        default:
            return "?";
    }
}

bool CopyMsvcString(void* message,
                    std::array<char, kMaxLogMessageLength + 1>& buffer,
                    std::size_t& copiedSize) {
    if (message == nullptr) {
        return false;
    }

    copiedSize = 0;

    __try {
        const auto* stringObject = reinterpret_cast<const std::uint8_t*>(
            message);
        std::size_t size = *reinterpret_cast<const std::size_t*>(
            stringObject + 16);

        if (size > kMaxLogMessageLength) {
            size = kMaxLogMessageLength;
        }

        const char* text = nullptr;
        if (size <= 15) {
            text = reinterpret_cast<const char*>(stringObject);
        } else {
            text = *reinterpret_cast<const char* const*>(stringObject);
        }

        if (text == nullptr) {
            return false;
        }

        std::memcpy(buffer.data(), text, size);
        buffer[size] = '\0';
        copiedSize = size;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void __fastcall HookedLog(void* context,
                          std::int32_t level,
                          void* message) {
    std::array<char, kMaxLogMessageLength + 1> buffer{};
    std::size_t copiedSize = 0;

    if (CopyMsvcString(message, buffer, copiedSize)) {
        std::ostringstream stream;
        stream << "[" << LogLevelName(level) << "] "
               << std::string_view(buffer.data(), copiedSize);
        LogInfo(stream.str());
    }

    if (g_originalLog != nullptr) {
        g_originalLog(context, level, message);
    }
}

bool IsMinHookInitialized(MH_STATUS status) {
    return status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED;
}

bool CheckMinHookStatus(MH_STATUS status, const char* action) {
    if (status == MH_OK) {
        return true;
    }

    std::ostringstream stream;
    stream << action << " failed with MinHook status "
           << static_cast<int>(status);
    LogError(stream.str());
    return false;
}

} // namespace

bool InstallLogHook() {
    MH_STATUS status = MH_Initialize();
    if (!IsMinHookInitialized(status)) {
        return CheckMinHookStatus(status, "MH_Initialize");
    }

    const std::uintptr_t logAddress =
        Patcher::FromRva(offsets::kLogFunctionRva);

    status = MH_CreateHook(
        reinterpret_cast<void*>(logAddress),
        reinterpret_cast<void*>(&HookedLog),
        reinterpret_cast<void**>(&g_originalLog));

    if (!CheckMinHookStatus(status, "MH_CreateHook")) {
        return false;
    }

    status = MH_EnableHook(reinterpret_cast<void*>(logAddress));
    if (!CheckMinHookStatus(status, "MH_EnableHook")) {
        return false;
    }

    LogInfo("Log hook installed.");
    return true;
}

} // namespace kraken::plugin
