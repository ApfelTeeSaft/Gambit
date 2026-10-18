#include "gui.hpp"
#include <chrono>
#include <iostream>
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
    for(int i=0;i<200 && !done;++i) {
        std::string part;
        done=session.consume(part,status);
        output+=part;
        if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if(!done || status==0 || output.find("does not exist")==std::string::npos)
        throw std::runtime_error("Session did not return the expected CLI error: " + output);
    std::cout<<"GUI process/session test passed\n";
}
