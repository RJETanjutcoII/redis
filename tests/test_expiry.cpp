#include <gtest/gtest.h>
#include "store.h"
#include "expiry.h"
#include <chrono>
#include <thread>

TEST(Expiry, JanitorDeletesExpiredKey) {
    KeyValueStore store;
    auto expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    store.set("foo", "bar", expiry);

    ExpiryJanitor janitor(store, 20);
    janitor.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    auto deletions = janitor.drain_deletions();
    store.delete_expired_batch(deletions);

    janitor.stop();

    EXPECT_FALSE(store.get("foo").has_value());
}

TEST(Expiry, JanitorLeavesLiveKey) {
    KeyValueStore store;
    auto expiry = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    store.set("foo", "bar", expiry);

    ExpiryJanitor janitor(store, 20);
    janitor.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    auto deletions = janitor.drain_deletions();
    store.delete_expired_batch(deletions);

    janitor.stop();

    EXPECT_TRUE(store.get("foo").has_value());
}
