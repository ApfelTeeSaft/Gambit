#include "gui.hpp"
#include "install.hpp"
#include <exception>
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
namespace {
void connect_console() {
    HANDLE current=GetStdHandle(STD_OUTPUT_HANDLE);
    if(!current || current==INVALID_HANDLE_VALUE) AttachConsole(ATTACH_PARENT_PROCESS);
    for(auto pair : {std::pair{STD_OUTPUT_HANDLE,stdout},std::pair{STD_ERROR_HANDLE,stderr}}) {
        HANDLE h=GetStdHandle(pair.first);
        if(!h || h==INVALID_HANDLE_VALUE)continue;
        HANDLE copy=nullptr;
        if(!DuplicateHandle(GetCurrentProcess(),h,GetCurrentProcess(),&copy,0,TRUE,DUPLICATE_SAME_ACCESS))continue;
        int fd=_open_osfhandle(reinterpret_cast<intptr_t>(copy),_O_TEXT);
        if(fd<0) {CloseHandle(copy);continue;}
        _dup2(fd,_fileno(pair.second));
        _close(fd);
    }
}
}
static int application_main(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif

    spl::Options opts;
    bool explicit_cli=false,force_gui=false,help=false;
    std::filesystem::path controls_directory;
    try {
        for(int i=1;i<argc;++i) {
            #ifdef _WIN32
            std::string arg;
            for(const wchar_t* it=argv[i];*it;++it) arg+=char(*it <= 127? *it : '?');
#else
            std::string arg=argv[i];
#endif
            auto value=[&](const char* option)->std::filesystem::path {
                if(i+1>=argc)throw std::runtime_error(std::string("Missing argument for ")+option);
                return std::filesystem::path(argv[++i]);
            };
            if(arg=="--cli")explicit_cli=true;
            else if(arg=="--gui")force_gui=true;
            else if(arg=="--rom") {opts.rom=value("--rom");explicit_cli=true;}
            else if(arg=="--repo") {opts.repository=value("--repo");explicit_cli=true;}
            else if(arg=="--output") {opts.output=value("--output");explicit_cli=true;}
            else if(arg=="--java") {opts.java=value("--java");explicit_cli=true;}
            else if(arg=="--jwud") {opts.jwud=value("--jwud");explicit_cli=true;}
            else if(arg=="--validate-only") {opts.validate_only=true;explicit_cli=true;}
            else if(arg=="--no-launch") {opts.no_launch=true;explicit_cli=true;}
            else if(arg=="--toolchain-only") {opts.toolchain_only=true;explicit_cli=true;}
            else if(arg=="--controls") controls_directory=value("--controls");
            else if(arg=="--help"||arg=="-h")help=true;
            else throw std::runtime_error("Unknown option: "+arg);
        }
        if(help) {
            std::cout<<"SplatoonSetup - graphical setup (default)\n"
                     <<"  --gui                 Force the graphical installer\n"
                     <<"  --cli                 Use the command-line setup\n"
                     <<"  --rom PATH            Dump folder or .zip/.wud/.wux image (CLI by default)\n"
                     <<"  --output PATH         Install directory\n"
                     <<"  --repo PATH           Release directory\n"
                     <<"  --validate-only       Check dump without installing\n"
                     <<"  --no-launch           Do not launch the game after installation\n"
                     <<"  --toolchain-only      Download portable toolchain only\n"
                     <<"  --controls PATH       Open controller rebinding editor for an installation\n"
                     <<"  --java PATH --jwud JAR  Optional WUX/WUD tools\n";
            return 0;
        }
        std::filesystem::path executable;
#ifdef _WIN32
        std::wstring path(32768,L'\0');
        DWORD length=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
        if(length>0&&length<path.size())executable=std::filesystem::path(path.substr(0,length));
#else
        std::string path(4096,'\0');
        auto length=readlink("/proc/self/exe",path.data(),path.size());
        if(length>0)executable=std::filesystem::path(path.substr(0,size_t(length)));
#endif
        if(executable.empty())executable=std::filesystem::absolute(std::filesystem::path(argv[0]));
        if(opts.repository.empty())opts.repository=executable.parent_path();
        opts.repository=std::filesystem::absolute(opts.repository);
        if(opts.output.empty())opts.output=opts.repository/"Game";
        if(!controls_directory.empty())return spl::open_controls_editor(controls_directory)?0:1;
        if(force_gui)return spl::run_gui(opts,executable);
        if(!explicit_cli)return spl::run_gui(opts,executable);
        std::cout.setf(std::ios::unitbuf);
        return spl::install(opts);
    } catch(const std::exception& e) {
        std::cerr<<"Setup failed: "<<e.what()<<'\n';return 1;
    }
}

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    int argc=0;
    wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(!argv)return 1;
    bool cli=argc>1;
    if(argc==2 && std::wstring(argv[1])==L"--gui")cli=false;
    if(cli)connect_console();
    int result=application_main(argc,argv);
    LocalFree(argv);
    return result;
}
#endif
