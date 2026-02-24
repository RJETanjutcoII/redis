#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <cstddef>

using RespCommand = std::vector<std::string>;

enum class RespError {
    NeedMoreData,
    ProtocolError,
};

struct ParseResult {
    RespCommand command;
    size_t bytes_consumed;
};

std::variant<ParseResult, RespError>
parse_resp(const char* buf, size_t len);

std::string resp_simple_string(std::string_view s);
std::string resp_error(std::string_view msg);
std::string resp_bulk_string(std::string_view s);
std::string resp_null_bulk();
std::string resp_integer(int64_t n);
std::string resp_array(const std::vector<std::string>& elements);
std::string resp_empty_array();
