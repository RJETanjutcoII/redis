#pragma once
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <shared_mutex>

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

class KeyValueStore {
public:
    void set(const std::string& key, std::string value, std::optional<TimePoint> expiry = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    void expire(const std::string& key, int64_t seconds);
    int64_t ttl(const std::string& key);
    void persist(const std::string& key);
    std::vector<std::string> keys(const std::string& pattern);
    size_t dbsize();
    void flushall();
    std::vector<std::string> sample_expired_keys(size_t max_samples);
    void delete_expired_batch(const std::vector<std::string>& keys);

private:
    struct Entry {
        std::string value;
        std::optional<TimePoint> expiry;
    };

    bool is_expired(const Entry& e) const;

    std::unordered_map<std::string, Entry> data_;
    std::unordered_map<std::string, TimePoint> expiry_index_; // only keys with a TTL
    mutable std::shared_mutex expiry_mutex_; // guards expiry_index_; data_ is single-threaded
};
