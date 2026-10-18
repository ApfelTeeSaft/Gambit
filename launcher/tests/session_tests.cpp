#include "gui.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <thread>

int main(int argc,char** argv) {
    if(argc!=2)throw std::runtime_error("Expected setup executable path");
    spl::Options options;
    options.repository=std::filesystem::absolute(std::filesystem::path(argv[1])).parent_path();
    options.output=options.repository/"Game";
    options.rom=options.repository/"nonexistent-session-test.rpx";
    spl::SetupSession session{std::filesystem::path(argv[1])};
    session.start(options,true);
    std::string output;
    int status=0;
    bool done=false;
    bool saw_progress=false;
    for(int i=0;i<200 && !done;++i) {
        std::string part;
        spl::SetupProgress progress;
        done=session.consume(part,status,&progress);
        if(progress.stage=="Validating game dump"&&progress.percent==-1)saw_progress=true;
        output+=part;
        if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if(!done || status==0 || !saw_progress || output.find("@@SPLPORT_PROGRESS")!=std::string::npos ||
       output.find("does not exist")==std::string::npos)
        throw std::runtime_error("Session did not return the expected CLI error: " + output);
    const auto& log_path=session.full_log_path();
    if(!std::filesystem::is_regular_file(log_path))
        throw std::runtime_error("Full setup log file was not created");
    std::ifstream file(log_path,std::ios::binary);
    std::string saved((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    if(saved.find("does not exist")==std::string::npos ||
       saved.find("@@SPLPORT_PROGRESS")!=std::string::npos)
        throw std::runtime_error("Full setup log did not retain the diagnostic output");
    std::cout<<"GUI process/session test passed\n";
}
