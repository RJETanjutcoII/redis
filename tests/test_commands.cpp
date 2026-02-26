#include <gtest/gtest.h>
#include "command.h"
#include "store.h"
#include "resp.h"

static CommandContext make_ctx(KeyValueStore& store) {
    return CommandContext{store, nullptr, "0.1.0"};
}

TEST(CommandTest, SetExInvalidInteger) {
    KeyValueStore store;
    auto ctx = make_ctx(store);
    CommandDispatcher dispatcher;
    auto result = dispatcher.dispatch(ctx, {"SET", "foo", "bar", "EX", "notanumber"});
    EXPECT_TRUE(result.rfind("-ERR", 0) == 0);
}

TEST(CommandTest, SetPxInvalidInteger) {
    KeyValueStore store;
    auto ctx = make_ctx(store);
    CommandDispatcher dispatcher;
    auto result = dispatcher.dispatch(ctx, {"SET", "foo", "bar", "PX", "notanumber"});
    EXPECT_TRUE(result.rfind("-ERR", 0) == 0);
}

TEST(CommandTest, ExpireInvalidInteger) {
    KeyValueStore store;
    auto ctx = make_ctx(store);
    CommandDispatcher dispatcher;
    dispatcher.dispatch(ctx, {"SET", "foo", "bar"});
    auto result = dispatcher.dispatch(ctx, {"EXPIRE", "foo", "notanumber"});
    EXPECT_TRUE(result.rfind("-ERR", 0) == 0);
}
