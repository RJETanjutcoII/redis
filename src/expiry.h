#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

class KeyValueStore;

class ExpiryJanitor {
public:
    explicit ExpiryJanitor(KeyValueStore& store, int interval_ms);
    ~ExpiryJanitor();

    void start();
    void stop();
    std::vector<std::string> drain_deletions();

private:
    void run();

    KeyValueStore& store_;
    int interval_ms_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::mutex mu_;
    std::vector<std::string> pending_deletions_;
};
