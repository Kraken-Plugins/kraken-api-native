#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace kraken::plugin {

class Patcher {
public:
    static bool Write(std::uintptr_t address,
                      std::span<const std::uint8_t> bytes);

    static bool Nop(std::uintptr_t address, std::size_t count);

    static bool VerifyBytes(std::uintptr_t address,
                            std::span<const std::uint8_t> expected);

    static std::uintptr_t FromRva(std::uintptr_t rva);
};

} // namespace kraken::plugin
