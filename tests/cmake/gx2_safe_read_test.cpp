#include "port/os/gx2/gx2_safe_read.h"
#include <array>
#include <cstdint>
#include <cstring>

int main() {
    const std::array<std::uint32_t, 4> source{0x12345678u, 0xabcdef01u, 0u, 0xffffffffu};
    std::array<std::uint32_t, 4> target{};
    if (!gx2_safe_read::copy(source.data(), target.data(), sizeof(source))) return 1;
    if (std::memcmp(source.data(), target.data(), sizeof(source)) != 0) return 2;
    if (!gx2_safe_read::copy(nullptr, nullptr, 0)) return 3;
    if (gx2_safe_read::copy(nullptr, target.data(), sizeof(target))) return 4;

#if defined(_WIN32)
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    const std::size_t page = system.dwPageSize;
    auto* region = static_cast<unsigned char*>(
        VirtualAlloc(nullptr, page * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!region) return 5;
    region[page - 1] = 0x5a;
    region[page] = 0xa5;
    unsigned char pair[2]{};
    if (!gx2_safe_read::copy(region + page - 1, pair, 2) ||
        pair[0] != 0x5a || pair[1] != 0xa5) {
        VirtualFree(region, 0, MEM_RELEASE);
        return 6;
    }
    DWORD old_protect = 0;
    if (!VirtualProtect(region + page, page, PAGE_NOACCESS, &old_protect)) {
        VirtualFree(region, 0, MEM_RELEASE);
        return 7;
    }
    // Even a partial read must invalidate a whole sweep.
    if (gx2_safe_read::copy(region + page - 1, pair, 2)) {
        VirtualFree(region, 0, MEM_RELEASE);
        return 8;
    }
    if (gx2_safe_read::copy(region + page, pair, 1)) {
        VirtualFree(region, 0, MEM_RELEASE);
        return 9;
    }
    VirtualFree(region, 0, MEM_RELEASE);
    if (gx2_safe_read::copy(region, pair, 1)) return 10;
#endif
    return 0;
}
