#pragma once
#include <bit>
#include <cstdint>

namespace LatteAddrLib {
constexpr std::uint32_t MostSignificantBitIndex32(std::uint32_t value) noexcept {
    return value == 0 ? 0 : static_cast<std::uint32_t>(std::bit_width(value) - 1);
}
}
