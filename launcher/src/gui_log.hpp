#pragma once
#include <cstddef>
#include <string>

namespace spl {
constexpr std::size_t max_gui_log_chars = 8 * 1024 * 1024;
constexpr std::size_t retained_gui_log_chars = 6 * 1024 * 1024;

template <typename Char>
bool append_gui_log(std::basic_string<Char>& log, const std::basic_string<Char>& text) {
    log.append(text);
    if(log.size() <= max_gui_log_chars) return false;
    std::size_t start = log.size() - retained_gui_log_chars;
    const auto newline = log.find(static_cast<Char>('\n'), start);
    if(newline != std::basic_string<Char>::npos) start = newline + 1;
    else if constexpr (sizeof(Char) == 1) {
        while(start < log.size() && (static_cast<unsigned char>(log[start]) & 0xc0) == 0x80) ++start;
    }
    log.erase(0, start);
    return true;
}
}
