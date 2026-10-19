#pragma once
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif
namespace gx2_cpu_features {
inline bool has_ssse3() noexcept {
#if defined(__clang__) || defined(__GNUC__)
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    return __builtin_cpu_supports("ssse3");
#else
    return false;
#endif
#elif defined(_MSC_VER)
    int cpu_info[4]{};
    __cpuid(cpu_info, 1);
    return (cpu_info[2] & (1 << 9)) != 0;
#else
    return false;
#endif
}
}
