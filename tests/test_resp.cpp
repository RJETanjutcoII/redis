#include <gtest/gtest.h>
#include "resp.h"

static ParseResult ok(const char* buf) {
    auto result = parse_resp(buf, strlen(buf));
    EXPECT_TRUE(std::holds_alternative<ParseResult>(result));
    return std::get<ParseResult>(result);
}

static RespError err(const char* buf) {
    auto result = parse_resp(buf, strlen(buf));
    EXPECT_TRUE(std::holds_alternative<RespError>(result));
    return std::get<RespError>(result);
}

TEST(RespParser, EmptyBuffer) {
    EXPECT_EQ(err(""), RespError::NeedMoreData);
}

TEST(RespParser, NotAnArray) {
    EXPECT_EQ(err("PING\r\n"), RespError::ProtocolError);
}

TEST(RespParser, CompletePing) {
    auto [cmd, consumed] = ok("*1\r\n$4\r\nPING\r\n");
    ASSERT_EQ(cmd.size(), 1u);
    EXPECT_EQ(cmd[0], "PING");
    EXPECT_EQ(consumed, 14u);
}

TEST(RespParser, CompleteSet) {
    auto [cmd, consumed] = ok("*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    ASSERT_EQ(cmd.size(), 3u);
    EXPECT_EQ(cmd[0], "SET");
    EXPECT_EQ(cmd[1], "foo");
    EXPECT_EQ(cmd[2], "bar");
}

TEST(RespParser, PartialHeader) {
    EXPECT_EQ(err("*3\r\n"), RespError::NeedMoreData);
}

TEST(RespParser, PartialBulkString) {
    EXPECT_EQ(err("*1\r\n$4\r\nPI"), RespError::NeedMoreData);
}

TEST(RespParser, SplitAtCrlf) {
    // First half ends right after the array header
    std::string first = "*1\r\n$4\r\n";
    EXPECT_EQ(err(first.c_str()), RespError::NeedMoreData);

    // Second half completes it
    std::string full = first + "PING\r\n";
    auto [cmd, consumed] = ok(full.c_str());
    ASSERT_EQ(cmd.size(), 1u);
    EXPECT_EQ(cmd[0], "PING");
}

TEST(RespParser, Pipelining) {
    std::string buf = "*1\r\n$4\r\nPING\r\n*1\r\n$4\r\nPING\r\n";
    const char* data = buf.data();
    size_t len = buf.size();

    auto r1 = parse_resp(data, len);
    ASSERT_TRUE(std::holds_alternative<ParseResult>(r1));
    auto [cmd1, consumed1] = std::get<ParseResult>(r1);
    EXPECT_EQ(cmd1[0], "PING");

    auto r2 = parse_resp(data + consumed1, len - consumed1);
    ASSERT_TRUE(std::holds_alternative<ParseResult>(r2));
    auto [cmd2, consumed2] = std::get<ParseResult>(r2);
    EXPECT_EQ(cmd2[0], "PING");
}

TEST(RespParser, ValueWithSpaces) {
    auto [cmd, consumed] = ok("*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$9\r\nhello bar\r\n");
    ASSERT_EQ(cmd.size(), 3u);
    EXPECT_EQ(cmd[2], "hello bar");
}
