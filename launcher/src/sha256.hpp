#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace spl::crypto {
std::string sha256(const std::filesystem::path& file);
}
