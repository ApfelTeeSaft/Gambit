#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace spl {
namespace fs = std::filesystem;
struct Options {
    fs::path repository;
    fs::path rom;
    fs::path output;
    fs::path java;
    fs::path jwud;
    bool validate_only = false;
    bool no_launch = false;
    bool toolchain_only = false;
};

bool supported_layout(const fs::path& root);
fs::path resolve_layout(const fs::path& root);
void validate_dump(const fs::path& root, bool check_hash = true);
void copy_game_tree(const fs::path& source, const fs::path& destination);
std::string select_input();
int execute(const fs::path& program, const std::vector<std::string>& args, const fs::path& cwd = {});
int install(const Options& options);
}
