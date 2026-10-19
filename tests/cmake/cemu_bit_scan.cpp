#include "Cafe/HW/Latte/LatteAddrLib/BitScan.h"
#include <array>
#include <cstdint>
#include <utility>

using LatteAddrLib::MostSignificantBitIndex32;

static_assert(MostSignificantBitIndex32(0u) == 0);
static_assert(MostSignificantBitIndex32(1u) == 0);
static_assert(MostSignificantBitIndex32(2u) == 1);
static_assert(MostSignificantBitIndex32(32u) == 5);
static_assert(MostSignificantBitIndex32(0x80000000u) == 31);
static_assert(MostSignificantBitIndex32(0xffffffffu) == 31);

int main() {
    constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 10> cases{{
        {0u, 0u}, {1u, 0u}, {2u, 1u}, {3u, 1u}, {4u, 2u},
        {16u, 4u}, {32u, 5u}, {256u, 8u},
        {0x80000000u, 31u}, {0xffffffffu, 31u}
    }};
    for (const auto& [value, expected] : cases) {
        if (MostSignificantBitIndex32(value) != expected) return 1;
    }
    return 0;
}
