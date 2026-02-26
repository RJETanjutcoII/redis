#include "expiry.h"
#include "store.h"
#include <chrono>

ExpiryJanitor::ExpiryJanitor(KeyValueStore& store, int interval_ms)
    : store_(store), interval_ms_(interval_ms) {}

ExpiryJanitor::~ExpiryJanitor() { stop(); }

void ExpiryJanitor::start() {
    running_ = true;
    thread_ = std::thread(&ExpiryJanitor::run, this);
}

void ExpiryJanitor::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

std::vector<std::string> ExpiryJanitor::drain_deletions() {
    std::lock_guard lock(mu_);
    std::vector<std::string> result;
    result.swap(pending_deletions_);
    return result;
}

void ExpiryJanitor::run() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
        auto expired = store_.sample_expired_keys(20);
        if (!expired.empty()) {
            std::lock_guard lock(mu_);
            pending_deletions_.insert(pending_deletions_.end(), expired.begin(), expired.end());
        }
    }
}
