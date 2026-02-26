#include <gtest/gtest.h>
#include "aof.h"
#include "store.h"
#include "command.h"
#include <fstream>
#include <cstdio>

static const char* TEST_AOF = "/tmp/test_mini_redis.aof";

class AofTest : public ::testing::Test {
protected:
    void SetUp() override { std::remove(TEST_AOF); }
    void TearDown() override { std::remove(TEST_AOF); }
};

TEST_F(AofTest, WriterCreatesFile) {
    AOFWriter writer(TEST_AOF, FsyncPolicy::No);
    EXPECT_TRUE(writer.is_open());
}

TEST_F(AofTest, AppendAndReplay) {
    {
        AOFWriter writer(TEST_AOF, FsyncPolicy::Always);
        writer.append({"SET", "foo", "bar"});
        writer.append({"SET", "hello", "world"});
    }

    AOFReplayer replayer(TEST_AOF);
    auto commands = replayer.replay();
    EXPECT_TRUE(replayer.error().empty());
    ASSERT_EQ(commands.size(), 2u);
    EXPECT_EQ(commands[0], (std::vector<std::string>{"SET", "foo", "bar"}));
    EXPECT_EQ(commands[1], (std::vector<std::string>{"SET", "hello", "world"}));
}

TEST_F(AofTest, ReplayRestoresStore) {
    {
        AOFWriter writer(TEST_AOF, FsyncPolicy::Always);
        writer.append({"SET", "foo", "bar"});
        writer.append({"SET", "x", "42"});
        writer.append({"DEL", "foo"});
    }

    KeyValueStore store;
    CommandDispatcher dispatcher;
    AOFReplayer replayer(TEST_AOF);
    auto commands = replayer.replay();
    CommandContext ctx{store, nullptr, "0.1.0"};
    for (auto& cmd : commands)
        dispatcher.dispatch(ctx, cmd);

    EXPECT_FALSE(store.get("foo").has_value());
    EXPECT_EQ(store.get("x").value(), "42");
}

TEST_F(AofTest, ReplayEmptyFile) {
    AOFReplayer replayer(TEST_AOF);
    auto commands = replayer.replay();
    EXPECT_EQ(commands.size(), 0u);
}
