#pragma once
#if defined(__clang__) || defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <cpuid.h>
#endif
#elif defined(_MSC_VER)
#include <intrin.h>
#endif
namespace gx2_cpu_features {
inline bool has_ssse3() noexcept {
#if defined(__clang__) || defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    unsigned int eax=0,ebx=0,ecx=0,edx=0;
    return __get_cpuid(1,&eax,&ebx,&ecx,&edx) != 0 && (ecx & (1u << 9)) != 0;
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
