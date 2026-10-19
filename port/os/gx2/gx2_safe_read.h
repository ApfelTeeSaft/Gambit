#pragma once
#include <cstddef>
#include <cstring>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace gx2_safe_read {
inline bool copy(const void* source, void* destination, std::size_t bytes) noexcept {
    if (bytes == 0) return true;
    if (!source || !destination) return false;
#if defined(_WIN32)
    SIZE_T copied = 0;
    return ReadProcessMemory(GetCurrentProcess(), source, destination, bytes, &copied) != 0 &&
           copied == bytes;
#else
    std::memcpy(destination, source, bytes);
    return true;
#endif
}
}
