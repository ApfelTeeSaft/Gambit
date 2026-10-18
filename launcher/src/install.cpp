#include "install.hpp"
#include "sha256.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <ShObjIdl.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace spl {
namespace {
constexpr auto supported_hash = "45ecd1ed81a97f46b1c8ae24820cd47fbc55399b50af09bf1f9050fe803308e5";
constexpr auto zig_version = "0.14.1";
#ifdef _WIN32
constexpr auto zig_platform = "x86_64-windows";
constexpr auto zig_sha = "554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c";
constexpr auto exe_suffix = ".exe";
#else
constexpr auto zig_platform = "x86_64-linux";
constexpr auto zig_sha = "24aeeec8af16c381934a6cd7d95c807a8cb2cf7df9fa40d359aa884195c4716c";
constexpr auto exe_suffix = "";
#endif

std::string lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));});
    return s;
}
void require(bool check, const std::string& message) {
    if(!check) throw std::runtime_error(message);
}
fs::path absolute_path(const fs::path& p) { return fs::absolute(p).lexically_normal(); }
std::string string(const fs::path& p) {
    auto utf8=p.u8string();
    return std::string(utf8.begin(),utf8.end());
}

#ifdef _WIN32
std::wstring quote_win(const std::wstring& argument) {
    std::wstring out=L"\"";
    unsigned slashes=0;
    for(wchar_t c:argument) {
        if(c==L'\\'){++slashes;continue;}
        if(c==L'\"'){
            out.append(slashes*2+1,L'\\');out+=c;
        } else {
            out.append(slashes,L'\\');out+=c;
        }
        slashes=0;
    }
    out.append(slashes*2,L'\\');out+=L'\"';
    return out;
}
#endif
bool exists_file(const fs::path& path){return fs::is_regular_file(path);}
fs::path locate(const fs::path& repo, const fs::path& relative, const std::string& name) {
    auto bundled = repo/relative/name;
    if(exists_file(bundled)) return absolute_path(bundled);
    auto local = repo/name;
    if(exists_file(local)) return absolute_path(local);
    return fs::path(name);
}
void run(const fs::path& program, std::vector<std::string> args, const fs::path& cwd={}) {
    auto result=execute(program,args,cwd);
    if(result!=0) {
        std::ostringstream s;s<<"Step failed: "<<program.filename().string()<<" returned "<<result;
        throw std::runtime_error(s.str());
    }
}
void download(const fs::path& repo, const std::string& url, const fs::path& destination, const std::string& sha) {
    if(exists_file(destination)) {
        if(sha.empty() || crypto::sha256(destination)==sha) return;
        fs::remove(destination);
    }
    fs::create_directories(destination.parent_path());
    auto partial=destination;partial+=".part";
    std::error_code ec; fs::remove(partial,ec);
    std::cout<<"Downloading "<<url<<"\n";
    try {
        run(locate(repo,"tools/curl",std::string("curl")+exe_suffix),{
            "--fail","--location","--proto","=https","--proto-redir","=https",
            "--tlsv1.2","--retry","3","--connect-timeout","30","--output",string(partial),url});
        if(!sha.empty()) require(crypto::sha256(partial)==sha,"Downloaded file did not match its pinned SHA-256 digest");
        fs::rename(partial,destination);
    } catch(...) { fs::remove(partial,ec);throw; }
}
void extract_archive(const fs::path& repo, const fs::path& archive, const fs::path& into) {
    require(fs::is_regular_file(archive),"Archive does not exist");
    fs::create_directories(into);
    // Prefer a bundled bsdtar where available; otherwise use host tar/unzip.
    // The launcher does not implicitly run any script from inside the archive.
    auto tar=locate(repo,"tools/bsdtar",std::string("bsdtar")+exe_suffix);
    if(tar==fs::path(std::string("bsdtar")+exe_suffix)) {
#ifdef _WIN32
        tar="tar.exe";
#else
        tar="tar";
#endif
    }
#ifndef _WIN32
    if(lower(archive.extension().string())==".zip" && tar==fs::path("tar")) {
        run("unzip",{"-q",string(absolute_path(archive)),"-d",string(absolute_path(into))});
        return;
    }
#endif
    run(tar,{"-xf",string(absolute_path(archive)),"-C",string(absolute_path(into))});
}
void stage_directory(const fs::path& source, const fs::path& output) {
    copy_game_tree(resolve_layout(source),output);
}
fs::path prepare_input(const Options& opts, const fs::path& work) {
    auto selected = absolute_path(opts.rom);
    require(fs::exists(selected),"Selected dump does not exist: " + string(selected));
    if(fs::is_directory(selected)) return resolve_layout(selected);
    auto ext=lower(selected.extension().string());
    require(ext==".zip" || ext==".wud" || ext==".wux",
            "Supported inputs: a raw dump directory, .zip, .wux, or .wud");
    auto destination=work/"extracted";
    if(fs::exists(destination)) fs::remove_all(destination);
    fs::create_directories(destination);
    if(ext==".zip") {
        extract_archive(opts.repository,selected,destination);
    } else {
        auto java = !opts.java.empty() ? opts.java : locate(opts.repository,"tools/java/bin",std::string("java")+exe_suffix);
        auto jwud = !opts.jwud.empty() ? opts.jwud : opts.repository/"tools/JWUDTool.jar";
        require(exists_file(jwud),"WUD/WUX extraction requires a local JWUDTool.jar in tools/ (or --jwud); no copyrighted keys are bundled");
        require(exists_file(java) || java==fs::path(std::string("java")+exe_suffix),
                "A portable Java runtime is needed in tools/java/bin/ or via --java");
        std::cout<<"Decrypting disc image using your local keys (e.g. game.key and common.key).\n";
        run(java,{"-jar",string(jwud),"-in",string(selected),"-out",string(absolute_path(destination)),"-decrypt"},selected.parent_path());
    }
    return resolve_layout(destination);
}
void bootstrap_zig(const fs::path& repo, const fs::path& work) {
    auto target=work/"toolchain"/"zig";
    auto zig=target/(std::string("zig")+exe_suffix);
    if(exists_file(zig)) {
        if(execute(zig,{"version"})==0) return;
        fs::remove_all(target);
    }
    auto name=std::string("zig-")+zig_platform+"-"+zig_version+
#ifdef _WIN32
        ".zip";
#else
        ".tar.xz";
#endif
    auto archive=work/"downloads"/name;
    auto url=std::string("https://ziglang.org/download/")+zig_version+"/"+name;
    download(repo,url,archive,zig_sha);
    auto temp=work/"toolchain"/"unpacking";
    fs::remove_all(temp);
    extract_archive(repo,archive,temp);
    auto extracted=temp/(std::string("zig-")+zig_platform+"-"+zig_version);
    require(exists_file(extracted/(std::string("zig")+exe_suffix)),"Expected Zig compiler executable not found in downloaded archive");
    fs::create_directories(target.parent_path());
    fs::rename(extracted,target);
    fs::remove_all(temp);
    run(zig,{"version"});
}
fs::path cmake_tool(const fs::path& repo) {return locate(repo,"tools/cmake/bin",std::string("cmake")+exe_suffix);}
fs::path ninja_tool(const fs::path& repo) {return locate(repo,"tools/ninja",std::string("ninja")+exe_suffix);}
void write_toolchain(const fs::path& work) {
    auto zig=absolute_path(work/"toolchain"/"zig"/(std::string("zig")+exe_suffix)).generic_string();
    std::ofstream out(work/"toolchain"/"zig.cmake",std::ios::binary);
    require(bool(out),"Unable to write Zig CMake toolchain");
    out<<"set(CMAKE_C_COMPILER \""<<zig<<"\" CACHE FILEPATH \"\" FORCE)\n"
       <<"set(CMAKE_C_COMPILER_ARG1 \"cc\" CACHE STRING \"\" FORCE)\n"
       <<"set(CMAKE_CXX_COMPILER \""<<zig<<"\" CACHE FILEPATH \"\" FORCE)\n"
       <<"set(CMAKE_CXX_COMPILER_ARG1 \"c++\" CACHE STRING \"\" FORCE)\n";
}
void prepare_game_folders(const fs::path& root) {
    for(auto name:{"save/common","mlc01/sys","mlc01/usr/title/00050000",
                  "mlc01/usr/title/0005000c","mlc01/usr/title/0005000e",
                  "mlc01/usr/save/00050000","mlc01/usr/boss"}) {
        fs::create_directories(root/name);
    }
}
size_t function_count(const fs::path& p) {
    std::ifstream in(p);
    size_t count=0;std::string line;
    while(std::getline(in,line)) {
        const auto pos=line.find("rbrew_register_func(");
        if(pos!=std::string::npos && line.find("//")!=0) ++count;
    }
    return count;
}
void build_private_game(const Options& opts, const fs::path& selected, const fs::path& work) {
    auto repo=absolute_path(opts.repository);
    auto cmake=cmake_tool(repo);
    auto ninja=ninja_tool(repo);
    require(execute(cmake,{"--version"})==0,"CMake missing. Use the published portable launcher bundle with tools/cmake/bin/.");
    require(execute(ninja,{"--version"})==0,"Ninja missing. Use the published portable launcher bundle with tools/ninja/.");
    write_toolchain(work);
    auto flags=std::vector<std::string>{"-G","Ninja","-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_MAKE_PROGRAM="+string(absolute_path(ninja)),
        "-DCMAKE_TOOLCHAIN_FILE="+string(absolute_path(work/"toolchain"/"zig.cmake"))};
    auto rebrewu=repo/(std::string("rebrewu")+exe_suffix);
    require(exists_file(rebrewu),
        "Bundled RebrewU binary is missing. Download the complete release ZIP (SplatoonSetup and rebrewu together).");
    auto generated=work/"generated"/"Gambit";
    auto entry=repo/"profiles"/"agmp-v16.entries";
    require(exists_file(entry),"Required supported version boundary profile missing");
    auto stamp=work/"generated"/"source.sha256";
    auto checksum=crypto::sha256(selected/"code"/"Gambit.rpx") + ":" + crypto::sha256(rebrewu)+":"+crypto::sha256(entry);
    std::string previous;{std::ifstream in(stamp);std::getline(in,previous);}
    if(previous!=checksum || !exists_file(generated/"Gambit_register.cpp") || function_count(generated/"Gambit_register.cpp")<96300) {
        fs::remove_all(generated);fs::create_directories(generated);
        run(rebrewu,{"recompile","--rpx",string(selected/"code"/"Gambit.rpx"),
                    "--output",string(generated),"--analyzer-entries",string(entry)});
        require(exists_file(generated/"Gambit_data.cpp") && exists_file(generated/"Gambit.h") &&
                function_count(generated/"Gambit_register.cpp")>=96300,
                "Recompiler output was incomplete. Refusing to compile an incomplete game.");
        std::ofstream(stamp)<<checksum<<'\n';
    }
    auto gameBuild=work/"game";
    auto gameArgs=std::vector<std::string>{"-S",string(repo/"port"),"-B",string(gameBuild)};
    gameArgs.insert(gameArgs.end(),flags.begin(),flags.end());
    gameArgs.push_back("-DGAMBIT_GENERATED_DIR="+string(generated));
    gameArgs.push_back("-DRBREW_BUILD_GAME=ON");
    gameArgs.push_back("-DSPLPORT_FETCH_DEPS=ON");
    run(cmake,gameArgs);
    run(cmake,{"--build",string(gameBuild),"--target","Gambit","--parallel","4"});
    auto binary=gameBuild/(std::string("Splatoon")+exe_suffix);
    if(!exists_file(binary)) binary=gameBuild/"Release"/(std::string("Splatoon")+exe_suffix);
    require(exists_file(binary),"Game executable was not produced");
    fs::create_directories(opts.output);
    fs::copy_file(binary,opts.output/binary.filename(),fs::copy_options::overwrite_existing);
    prepare_game_folders(opts.output);
    std::cout<<"Private game build completed in "<<string(opts.output)<<"\n";
}
}

int execute(const fs::path& program, const std::vector<std::string>& args, const fs::path& cwd) {
#ifdef _WIN32
    std::wstring command=quote_win(program.wstring());
    for(const auto& a:args) {
        int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,a.data(),int(a.size()),nullptr,0);
        if(n==0)return -int(GetLastError());
        std::wstring wide(n,L'\0');
        MultiByteToWideChar(CP_UTF8,0,a.data(),int(a.size()),wide.data(),n);
        command+=L" "+quote_win(wide);
    }
    std::vector<wchar_t> mutable_cmd(command.begin(),command.end());mutable_cmd.push_back(0);
    STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    auto working=cwd.empty()?L"":absolute_path(cwd).wstring();
    auto path=program.wstring();
    if(!CreateProcessW(nullptr,mutable_cmd.data(),nullptr,nullptr,FALSE,0,nullptr,
                       working.empty()?nullptr:working.c_str(),&si,&pi)) return -int(GetLastError());
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD result=0;GetExitCodeProcess(pi.hProcess,&result);
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
    return int(result);
#else
    pid_t id=fork();if(id<0) return -1;
    if(id==0) {
        if(!cwd.empty() && chdir(cwd.c_str())!=0) _exit(127);
        std::string prog=program.string();
        std::vector<std::string> owned{prog};owned.insert(owned.end(),args.begin(),args.end());
        std::vector<char*> argv;for(auto& a:owned) argv.push_back(a.data());argv.push_back(nullptr);
        execvp(prog.c_str(),argv.data());_exit(127);
    }
    int status=0;if(waitpid(id,&status,0)<0) return -1;
    if(WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
#endif
}

bool supported_layout(const fs::path& root) {
    return fs::is_regular_file(root/"code"/"Gambit.rpx") &&
           fs::is_directory(root/"content") && fs::is_regular_file(root/"meta"/"meta.xml");
}
fs::path resolve_layout(const fs::path& root) {
    if(supported_layout(root)) return absolute_path(root);
    // Disc extraction can produce a wrapper directory. Search only a few levels.
    std::vector<fs::path> candidates;
    if(fs::is_directory(root)) {
        auto it=fs::recursive_directory_iterator(root,fs::directory_options::skip_permission_denied);
        for(;it!=fs::recursive_directory_iterator();++it) {
            if(it.depth()>3) {it.disable_recursion_pending();continue;}
            if(it->is_directory() && supported_layout(it->path())) candidates.push_back(it->path());
            if(candidates.size()>1) break;
        }
    }
    require(candidates.size()==1,"Dump must contain exactly one supported code/content/meta layout (possibly in a wrapper folder)");
    return absolute_path(candidates.front());
}
void validate_dump(const fs::path& root, bool check_hash) {
    require(supported_layout(root),"Dump missing code/Gambit.rpx, content/, or meta/meta.xml");
    if(check_hash) require(crypto::sha256(root/"code"/"Gambit.rpx")==supported_hash,
       "Unsupported RPX. Only Splatoon EUR AGMP01 update v16 (288) is supported; no guessed versions are allowed");
}
void copy_game_tree(const fs::path& source, const fs::path& destination) {
    require(supported_layout(source),"Missing game folders in source");
    require(absolute_path(source)!=absolute_path(destination),"Source and destination must differ");
    const auto source_path=absolute_path(source);
    const auto destination_path=absolute_path(destination);
    require(std::mismatch(source_path.begin(),source_path.end(),
                          destination_path.begin(),destination_path.end()).first!=source_path.end(),
            "Destination cannot be nested inside the selected ROM source");
    // Symlinks are not followed: only regular files in the user's own dump are copied.
    for(auto dirname:{"code","content","meta"}) {
        auto root=source/dirname;
        for(auto it=fs::recursive_directory_iterator(root,fs::directory_options::none);it!=fs::recursive_directory_iterator();++it) {
            require(!fs::is_symlink(it->symlink_status()),"Source contains a symlink; refusing to follow it");
            auto relative=fs::relative(it->path(),source);
            auto target=destination/relative;
            if(it->is_directory()) fs::create_directories(target);
            else if(it->is_regular_file()) {
                fs::create_directories(target.parent_path());
                fs::copy_file(it->path(),target,fs::copy_options::overwrite_existing);
            } else throw std::runtime_error("Unsupported file type inside dump");
        }
    }
}

std::string select_input() {
#ifdef _WIN32
    if(SUCCEEDED(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED))) {
        std::cout<<"Choose (1) a ROM file or (2) a raw dump folder: ";
        int kind=1;std::cin>>kind;
        IFileOpenDialog* dialog=nullptr;
        if(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog)))) {
            DWORD flags=0;dialog->GetOptions(&flags);
            dialog->SetOptions(flags|FOS_FORCEFILESYSTEM|(kind==2?FOS_PICKFOLDERS:0));
            std::string path;
            if(SUCCEEDED(dialog->Show(nullptr))) {
                IShellItem* item=nullptr;
                if(SUCCEEDED(dialog->GetResult(&item))) {
                    PWSTR raw=nullptr;
                    if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&raw))) {
                        path=fs::path(raw).string();CoTaskMemFree(raw);
                    }
                    item->Release();
                }
            }
            dialog->Release();CoUninitialize();
            if(!path.empty()) return path;
        } else CoUninitialize();
    }
#else
    std::cout<<"Choose (1) a ROM file or (2) a raw dump folder: ";
    int kind=1;std::cin>>kind;
    // Static arguments: no user-provided data is interpolated into the command.
    FILE* picker=popen(kind==2?"zenity --file-selection --directory --title=Select\\ game\\ dump 2>/dev/null"
                              :"zenity --file-selection --title=Select\\ ROM\\ image 2>/dev/null","r");
    if(picker) {
        char buffer[8192]{};
        std::string path;
        while(fgets(buffer,sizeof(buffer),picker)) path+=buffer;
        auto status=pclose(picker);
        if(status==0) {
            while(!path.empty() && (path.back()=='\n' || path.back()=='\r')) path.pop_back();
            if(!path.empty()) return path;
        }
    }
#endif
    std::cout<<"Enter the full path to your game dump folder or image: ";
    std::cin>>std::ws;std::string selected;std::getline(std::cin,selected);
    if(selected.size()>1 && selected.front()=='"' && selected.back()=='"') selected=selected.substr(1,selected.size()-2);
    return selected;
}
int install(const Options& user) {
    Options opts=user;
    opts.repository=absolute_path(opts.repository.empty()?fs::current_path():opts.repository);
    if(opts.output.empty()) opts.output=opts.repository/"Game";
    opts.output=absolute_path(opts.output);
    auto work=opts.repository/"build";
    if(opts.toolchain_only) {bootstrap_zig(opts.repository,work);return 0;}
    if(opts.rom.empty() && exists_file(opts.output/(std::string("Splatoon")+exe_suffix))) {
        if(!opts.no_launch) return execute(opts.output/(std::string("Splatoon")+exe_suffix),{},opts.output);
        return 0;
    }
    if(opts.rom.empty()) opts.rom=select_input();
    require(!opts.rom.empty(),"No ROM dump selected");
    fs::path source;
    if(opts.validate_only && fs::is_directory(opts.rom)) source=resolve_layout(opts.rom);
    else source=prepare_input(opts,work);
    validate_dump(source);
    std::cout<<"Validated supported Splatoon EUR v16 dump.\n";
    if(opts.validate_only) return 0;
    std::cout<<"This setup downloads an official Zig compiler and builds game-derived code locally.\n";
    bootstrap_zig(opts.repository,work);
    stage_directory(source,opts.output);
    build_private_game(opts,source,work);
    if(!opts.no_launch) return execute(opts.output/(std::string("Splatoon")+exe_suffix),{},opts.output);
    return 0;
}
}
