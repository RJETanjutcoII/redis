#include "store.h"
#include <fnmatch.h>
#include <mutex>
#include <shared_mutex>

bool KeyValueStore::is_expired(const Entry& e) const {
    if (!e.expiry.has_value()) return false;
    return Clock::now() >= e.expiry.value();
}

void KeyValueStore::set(const std::string& key, std::string value, std::optional<TimePoint> expiry) {
    data_[key] = Entry{std::move(value), expiry};

    std::unique_lock lock(expiry_mutex_);
    if (expiry.has_value()) {
        expiry_index_[key] = expiry.value();
    } else {
        expiry_index_.erase(key);
    }
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;

    if (is_expired(it->second)) {
        std::unique_lock lock(expiry_mutex_);
        expiry_index_.erase(key);
        data_.erase(it);
        return std::nullopt;
    }

    return it->second.value;
}

bool KeyValueStore::del(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) return false;

    std::unique_lock lock(expiry_mutex_);
    expiry_index_.erase(key);
    data_.erase(it);
    return true;
}

void KeyValueStore::expire(const std::string& key, int64_t seconds) {
    auto it = data_.find(key);
    if (it == data_.end()) return;

    auto expiry = Clock::now() + std::chrono::seconds(seconds);
    it->second.expiry = expiry;

    std::unique_lock lock(expiry_mutex_);
    expiry_index_[key] = expiry;
}

int64_t KeyValueStore::ttl(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) return -2;
    if (is_expired(it->second)) return -2;
    if (!it->second.expiry.has_value()) return -1;

    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        it->second.expiry.value() - Clock::now()
    ).count();

    return remaining > 0 ? remaining : -2;
}

void KeyValueStore::persist(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) return;

    it->second.expiry = std::nullopt;

    std::unique_lock lock(expiry_mutex_);
    expiry_index_.erase(key);
}

std::vector<std::string> KeyValueStore::keys(const std::string& pattern) {
    std::vector<std::string> result;
    for (auto& [k, entry] : data_) {
        if (is_expired(entry)) continue;
        if (fnmatch(pattern.c_str(), k.c_str(), 0) == 0)
            result.push_back(k);
    }
    return result;
}

size_t KeyValueStore::dbsize() {
    size_t count = 0;
    for (auto& [k, entry] : data_) {
        if (!is_expired(entry)) ++count;
    }
    return count;
}

void KeyValueStore::flushall() {
    std::unique_lock lock(expiry_mutex_);
    data_.clear();
    expiry_index_.clear();
}

std::vector<std::string> KeyValueStore::sample_expired_keys(size_t max_samples) {
    std::vector<std::string> expired;
    size_t checked = 0;

    std::shared_lock lock(expiry_mutex_);
    for (auto& [key, expiry] : expiry_index_) {
        if (checked++ >= max_samples) break;
        if (Clock::now() >= expiry)
            expired.push_back(key);
    }
    return expired;
}

void KeyValueStore::delete_expired_batch(const std::vector<std::string>& keys) {
    std::unique_lock lock(expiry_mutex_);
    for (const auto& key : keys) {
        data_.erase(key);
        expiry_index_.erase(key);
    }
}
