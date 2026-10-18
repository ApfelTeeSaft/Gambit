#include "gui.hpp"
#include "gui_log.hpp"
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace spl {
namespace {
std::string utf8(const fs::path& path) {
    auto bytes=path.u8string();
    return std::string(bytes.begin(),bytes.end());
}
#ifdef _WIN32
std::wstring wide(const std::string& s) {
    if(s.empty()) return {};
    int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);
    if(!length) throw std::runtime_error("Invalid UTF-8 input");
    std::wstring result(length,L'\0');
    MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),result.data(),length);
    return result;
}
std::wstring quote(const std::wstring& s) {
    std::wstring output=L"\"";
    size_t backslashes=0;
    for(wchar_t ch:s) {
        if(ch==L'\\') {++backslashes;continue;}
        if(ch==L'"') {output.append(backslashes*2+1,L'\\');output+=ch;}
        else {output.append(backslashes,L'\\');output+=ch;}
        backslashes=0;
    }
    output.append(backslashes*2,L'\\');output+=L'"';
    return output;
}
#endif
}
SetupSession::SetupSession(fs::path executable):executable_(std::move(executable)){}
SetupSession::~SetupSession() {
    cancel();
    if(thread_.joinable()) thread_.join();
#ifdef _WIN32
    { std::lock_guard<std::mutex> lock(process_mutex_);
      if(job_) CloseHandle(static_cast<HANDLE>(job_)); }
#endif
}
void SetupSession::push(std::string message) {
    std::lock_guard<std::mutex> lock(mutex_);
    for(char ch:message) {
        if(ch!='\n') {
            if(incoming_line_.size()==262144) {
                pending_+=incoming_line_;
                incoming_line_.clear();
            }
            incoming_line_.push_back(ch);
            continue;
        }
        if(!incoming_line_.empty()&&incoming_line_.back()=='\r')incoming_line_.pop_back();
        constexpr auto marker="@@SPLPORT_PROGRESS\t";
        if(incoming_line_.rfind(marker,0)==0) {
            auto tab=incoming_line_.find('\t',sizeof("@@SPLPORT_PROGRESS\t")-1);
            if(tab!=std::string::npos) {
                try {
                    int percent=std::stoi(incoming_line_.substr(tab+1));
                    if(percent>=-1&&percent<=100&&tab<160) {
                        progress_={incoming_line_.substr(sizeof("@@SPLPORT_PROGRESS\t")-1,
                                                          tab-(sizeof("@@SPLPORT_PROGRESS\t")-1)),percent};
                        progress_changed_=true;
                    }
                } catch(const std::exception&) {}
            }
        } else {
            pending_+=incoming_line_;
            pending_+='\n';
        }
        incoming_line_.clear();
    }
    if(pending_.size()>max_gui_log_chars) {
        std::size_t drop=pending_.size()-retained_gui_log_chars;
        auto boundary=pending_.find('\n',drop);
        if(boundary!=std::string::npos)drop=boundary+1;
        pending_.erase(0,drop);
    }
}
void SetupSession::start(const Options& opts, bool validate_only) {
    if(busy_.load()) throw std::runtime_error("Setup is already running");
    if(thread_.joinable()) thread_.join();
    cancelling_=false;
    finished_=false;
    exit_code_=-1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.clear();
        incoming_line_.clear();
        progress_={};
        progress_changed_=false;
    }
    std::vector<std::string> args{"--cli","--rom",utf8(opts.rom),"--repo",utf8(opts.repository),
                                  "--output",utf8(opts.output),"--no-launch"};
    if(validate_only) args.push_back("--validate-only");
    if(!opts.java.empty()) {args.push_back("--java");args.push_back(utf8(opts.java));}
    if(!opts.jwud.empty()) {args.push_back("--jwud");args.push_back(utf8(opts.jwud));}
    busy_=true;
    try { thread_=std::thread([this,args=std::move(args)]{worker(args);}); }
    catch(...) {busy_=false;throw;}
}
void SetupSession::cancel() {
    cancelling_=true;
#ifdef _WIN32
    { std::lock_guard<std::mutex> lock(process_mutex_);
      if(job_) TerminateJobObject(static_cast<HANDLE>(job_),1); }
#else
    int id=pid_.load();
    if(id>0) {
        if(kill(-id,SIGTERM)!=0 && errno==ESRCH) kill(id,SIGTERM);
    }
#endif
}
bool SetupSession::consume(std::string& log, int& result, SetupProgress* progress) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        log.swap(pending_);
        if(progress) {
            *progress=progress_changed_?progress_:SetupProgress{};
            progress_changed_=false;
        }
    }
    if(finished_.exchange(false)) {
        if(thread_.joinable()) thread_.join();
        result=exit_code_;
        return true;
    }
    return false;
}
bool SetupSession::can_launch(const fs::path& output) const {
#ifdef _WIN32
    return fs::is_regular_file(output/"Splatoon.exe");
#else
    return fs::is_regular_file(output/"Splatoon");
#endif
}
void SetupSession::worker(std::vector<std::string> args) {
    int status=-1;
#ifdef _WIN32
    HANDLE reader=nullptr,writer=nullptr;
    SECURITY_ATTRIBUTES sec{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
    if(CreatePipe(&reader,&writer,&sec,0)) {
        SetHandleInformation(reader,HANDLE_FLAG_INHERIT,0);
        STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;
        si.hStdOutput=writer;si.hStdError=writer;
        si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
        HANDLE null_input=nullptr;
        if(!si.hStdInput || si.hStdInput==INVALID_HANDLE_VALUE) {
            null_input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   &sec,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            si.hStdInput=null_input;
        }
        PROCESS_INFORMATION pi{};
        std::wstring cmd=quote(executable_.wstring());
        try {for(auto& arg:args) cmd+=L" "+quote(wide(arg));}
        catch(const std::exception& e) {push(std::string(e.what())+"\n");}
        std::vector<wchar_t> buffer(cmd.begin(),cmd.end());buffer.push_back(0);
        HANDLE job=CreateJobObjectW(nullptr,nullptr);
        if(job) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
            limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits));
        }
        // CREATE_SUSPENDED permits assignment before the child can spawn tools.
        BOOL ok=CreateProcessW(executable_.c_str(),buffer.data(),nullptr,nullptr,TRUE,
                               CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,nullptr,&si,&pi);
        CloseHandle(writer);
        if(null_input && null_input!=INVALID_HANDLE_VALUE)CloseHandle(null_input);
        if(ok) {
            { std::lock_guard<std::mutex> lock(process_mutex_); job_=job; }
            if(job && !AssignProcessToJobObject(job,pi.hProcess)) {
                TerminateProcess(pi.hProcess,1);
                push("Unable to attach the setup process to its cancellation job.\n");
            } else ResumeThread(pi.hThread);
            CloseHandle(pi.hThread);
            if(cancelling_) cancel();
            char data[4096];DWORD got=0;
            while(ReadFile(reader,data,sizeof(data),&got,nullptr) && got) push(std::string(data,got));
            WaitForSingleObject(pi.hProcess,INFINITE);
            DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);status=int(code);
            CloseHandle(pi.hProcess);
        } else {
            push("Unable to launch setup process (Windows error "+std::to_string(GetLastError())+").\n");
        }
        if(job) {
            std::lock_guard<std::mutex> lock(process_mutex_);
            CloseHandle(job);job_=nullptr;
        }
        CloseHandle(reader);
    } else push("Unable to create process output pipe.\n");
#else
    int fds[2];
    if(pipe(fds)==0) {
        pid_t child=fork();
        if(child==0) {
            setpgid(0,0);
            close(fds[0]);
            dup2(fds[1],STDOUT_FILENO);
            dup2(fds[1],STDERR_FILENO);
            close(fds[1]);
            std::string program=executable_.string();
            std::vector<char*> argv{program.data()};
            for(auto& a:args) argv.push_back(a.data());
            argv.push_back(nullptr);
            execv(program.c_str(),argv.data());
            _exit(127);
        }
        close(fds[1]);
        if(child>0) {
            setpgid(child,child);
            pid_=child;
            if(cancelling_) kill(-child,SIGTERM);
            char buffer[4096];ssize_t got;
            while((got=read(fds[0],buffer,sizeof(buffer)))>0) push(std::string(buffer,size_t(got)));
            int wait_status=0;
            if(waitpid(child,&wait_status,0)>0 && WIFEXITED(wait_status)) status=WEXITSTATUS(wait_status);
            pid_=-1;
        } else push("Unable to start setup process.\n");
        close(fds[0]);
    } else push("Unable to create setup process pipe.\n");
#endif
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!incoming_line_.empty()) {
            pending_+=incoming_line_;
            pending_+='\n';
            incoming_line_.clear();
        }
    }
    if(cancelling_) push("\nSetup cancelled.\n");
    exit_code_=status;
    busy_=false;
    finished_=true;
}
}
