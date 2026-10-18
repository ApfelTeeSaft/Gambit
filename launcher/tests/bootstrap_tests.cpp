#include "install.hpp"
#include "gui_log.hpp"
#include "sha256.hpp"
#include <stdexcept>
#define CHECK(v) do { if(!(v)) throw std::runtime_error("Check failed: " #v); } while(false)
#include <chrono>
#include <fstream>
#include <iostream>

int main() {
    std::wstring mixed=L"first\nsecond\r\nthird\rfourth";
    CHECK(spl::windows_log_newlines(mixed)==L"first\r\nsecond\r\nthird\r\nfourth");
    std::string log="first\n";
    CHECK(!spl::append_gui_log(log,std::string("second\n")));
    CHECK(log=="first\nsecond\n");
    CHECK(!spl::append_gui_log(log,std::string(spl::max_gui_log_chars-log.size(),'x')));
    CHECK(spl::append_gui_log(log,std::string("\nlatest\n")));
    CHECK(log=="latest\n");
    std::wstring wide_log(spl::max_gui_log_chars,L'x');
    CHECK(spl::append_gui_log(wide_log,std::wstring(L"\nlatest\n")));
    CHECK(wide_log==L"latest\n");
    namespace fs=std::filesystem;
    auto root=fs::temp_directory_path()/("splbootstrap-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        fs::create_directories(root/"input"/"code");
        fs::create_directories(root/"input"/"content");
        fs::create_directories(root/"input"/"meta");
        std::ofstream(root/"input"/"code"/"Gambit.rpx")<<"abc";
        std::ofstream(root/"input"/"meta"/"meta.xml")<<"<meta/>";
        std::ofstream(root/"input"/"content"/"test.txt")<<"data";
        CHECK(spl::crypto::sha256(root/"input"/"code"/"Gambit.rpx")==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        bool got_output=false;
        CHECK(spl::execute("cmake",{"-E","echo","splatoon-child-output"},{},
              [&](const std::string& line) {
                  if(line=="splatoon-child-output")got_output=true;
                  return false;
              })==0);
        CHECK(got_output);
        std::string lines;
        CHECK(spl::execute("cmake",{"-E","echo","first\n\nthird"},{},
              [&](const std::string& line) {
                  lines+=line+"|";
                  return false;
              })==0);
        CHECK(lines=="first||third|");
        CHECK(spl::supported_layout(root/"input"));
        CHECK(spl::resolve_layout(root)==fs::absolute(root/"input"));
        spl::validate_dump(root/"input",false);
        spl::copy_game_tree(root/"input",root/"output");
        CHECK(fs::exists(root/"output"/"content"/"test.txt"));
        bool nested_rejected=false;
        try {spl::copy_game_tree(root/"input",root/"input"/"nested");}
        catch(const std::exception&){nested_rejected=true;}
        CHECK(nested_rejected);
        spl::copy_game_tree(root/"input",root/"input-other");
        CHECK(fs::exists(root/"input-other"/"content"/"test.txt"));
        bool rejected=false;
        try {spl::validate_dump(root/"input");}catch(const std::exception&){rejected=true;}
        CHECK(rejected);
        fs::remove_all(root);
        std::cout<<"Bootstrap tests passed\n";
        return 0;
    } catch(...) {fs::remove_all(root);throw;}
}
