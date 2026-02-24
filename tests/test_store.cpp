#include <gtest/gtest.h>
#include "store.h"
#include <chrono>

TEST(Store, SetAndGet) {
    KeyValueStore store;
    store.set("foo", "bar");
    auto val = store.get("foo");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "bar");
}

TEST(Store, GetMissing) {
    KeyValueStore store;
    EXPECT_FALSE(store.get("missing").has_value());
}

TEST(Store, Overwrite) {
    KeyValueStore store;
    store.set("foo", "bar");
    store.set("foo", "baz");
    EXPECT_EQ(store.get("foo").value(), "baz");
}

TEST(Store, Del) {
    KeyValueStore store;
    store.set("foo", "bar");
    EXPECT_TRUE(store.del("foo"));
    EXPECT_FALSE(store.get("foo").has_value());
}

TEST(Store, DelMissing) {
    KeyValueStore store;
    EXPECT_FALSE(store.del("missing"));
}

TEST(Store, Dbsize) {
    KeyValueStore store;
    EXPECT_EQ(store.dbsize(), 0u);
    store.set("a", "1");
    store.set("b", "2");
    EXPECT_EQ(store.dbsize(), 2u);
    store.del("a");
    EXPECT_EQ(store.dbsize(), 1u);
}

TEST(Store, Flushall) {
    KeyValueStore store;
    store.set("a", "1");
    store.set("b", "2");
    store.flushall();
    EXPECT_EQ(store.dbsize(), 0u);
}

TEST(Store, KeysWildcard) {
    KeyValueStore store;
    store.set("foo", "1");
    store.set("bar", "2");
    store.set("baz", "3");
    auto result = store.keys("*");
    EXPECT_EQ(result.size(), 3u);
}

TEST(Store, KeysPattern) {
    KeyValueStore store;
    store.set("foo", "1");
    store.set("bar", "2");
    store.set("baz", "3");
    auto result = store.keys("ba*");
    EXPECT_EQ(result.size(), 2u);
}

TEST(Store, TtlNoExpiry) {
    KeyValueStore store;
    store.set("foo", "bar");
    EXPECT_EQ(store.ttl("foo"), -1);
}

TEST(Store, TtlMissing) {
    KeyValueStore store;
    EXPECT_EQ(store.ttl("missing"), -2);
}

TEST(Store, ExpireAndTtl) {
    KeyValueStore store;
    store.set("foo", "bar");
    store.expire("foo", 10);
    int64_t t = store.ttl("foo");
    EXPECT_GT(t, 0);
    EXPECT_LE(t, 10);
}

TEST(Store, LazyExpiry) {
    KeyValueStore store;
    store.set("foo", "bar", Clock::now() - std::chrono::seconds(1));
    EXPECT_FALSE(store.get("foo").has_value());
}

TEST(Store, Persist) {
    KeyValueStore store;
    store.set("foo", "bar");
    store.expire("foo", 10);
    store.persist("foo");
    EXPECT_EQ(store.ttl("foo"), -1);
}
