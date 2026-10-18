#pragma once
#include "install.hpp"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace spl {
class SetupSession {
public:
    explicit SetupSession(fs::path executable);
    ~SetupSession();
    SetupSession(const SetupSession&) = delete;
    SetupSession& operator=(const SetupSession&) = delete;
    void start(const Options& options, bool validate_only);
    void cancel();
    bool running() const { return busy_.load(); }
    bool consume(std::string& log, int& result);
    bool can_launch(const fs::path& output) const;
private:
    void worker(std::vector<std::string> args);
    void push(std::string message);
    fs::path executable_;
    std::thread thread_;
    std::mutex mutex_;
    std::string pending_;
    std::atomic<bool> busy_{false};
    std::atomic<bool> finished_{false};
    std::atomic<bool> cancelling_{false};
    int exit_code_ = -1;
#ifdef _WIN32
    void* job_ = nullptr;
    std::mutex process_mutex_;
#else
    std::atomic<int> pid_{-1};
#endif
};
int run_gui(const Options& options, const fs::path& executable);
}
