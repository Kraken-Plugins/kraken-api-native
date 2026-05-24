#include "patcher.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "logger.hpp"

#include <Windows.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace kraken::plugin {
namespace {

std::string HexAddress(std::uintptr_t address) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << address;
    return stream.str();
}

void LogWin32Failure(const char* action, std::uintptr_t address) {
    std::ostringstream stream;
    stream << action << " failed at " << HexAddress(address)
           << " with Win32 error " << GetLastError();
    LogError(stream.str());
}

} // namespace

bool Patcher::Write(std::uintptr_t address,
                    std::span<const std::uint8_t> bytes) {
    if (address == 0) {
        LogError("Patch address is null.");
        return false;
    }

    if (bytes.empty()) {
        LogError("Patch byte sequence is empty.");
        return false;
    }

    DWORD oldProtect = 0;
    void* target = reinterpret_cast<void*>(address);
    const SIZE_T size = bytes.size();

    if (!VirtualProtect(target, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LogWin32Failure("VirtualProtect", address);
        return false;
    }

    std::memcpy(target, bytes.data(), bytes.size());

    bool success = true;
    if (!FlushInstructionCache(GetCurrentProcess(), target, size)) {
        LogWin32Failure("FlushInstructionCache", address);
        success = false;
    }

    DWORD ignoredProtect = 0;
    if (!VirtualProtect(target, size, oldProtect, &ignoredProtect)) {
        LogWin32Failure("VirtualProtect restore", address);
        success = false;
    }

    if (success) {
        std::ostringstream stream;
        stream << "Patched " << bytes.size() << " bytes at "
               << HexAddress(address);
        LogInfo(stream.str());
    }

    return success;
}

bool Patcher::Nop(std::uintptr_t address, std::size_t count) {
    std::vector<std::uint8_t> bytes(count, 0x90);
    return Write(address, bytes);
}

bool Patcher::VerifyBytes(std::uintptr_t address,
                          std::span<const std::uint8_t> expected) {
    if (address == 0 || expected.empty()) {
        return false;
    }

    const auto* actual = reinterpret_cast<const std::uint8_t*>(address);
    return std::memcmp(actual, expected.data(), expected.size()) == 0;
}

std::uintptr_t Patcher::FromRva(std::uintptr_t rva) {
    return reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)) + rva;
}

} // namespace kraken::plugin
