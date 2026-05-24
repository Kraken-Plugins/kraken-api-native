#pragma once

#include <cstdint>

namespace kraken::plugin::offsets {

constexpr std::uintptr_t kInitialNopPatchRva = 0xE7189;
constexpr std::uintptr_t kLogFunctionRva = 0xA1F790;

} // namespace kraken::plugin::offsets
