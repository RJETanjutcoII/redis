#include "resp.h"
#include <charconv>

static size_t find_crlf(const char* buf, size_t len, size_t pos) {
    while (pos + 1 < len) {
        if (buf[pos] == '\r' && buf[pos + 1] == '\n') return pos;
        ++pos;
    }
    return std::string::npos;
}

static bool parse_int(const char* buf, size_t start, size_t end, int64_t& out) {
    auto [ptr, ec] = std::from_chars(buf + start, buf + end, out);
    return ec == std::errc{} && ptr == buf + end;
}

std::variant<ParseResult, RespError>
parse_resp(const char* buf, size_t len) {
    if (len == 0) return RespError::NeedMoreData;
    if (buf[0] != '*') return RespError::ProtocolError;

    size_t pos = 1;

    size_t crlf = find_crlf(buf, len, pos);
    if (crlf == std::string::npos) return RespError::NeedMoreData;

    int64_t num_elements = 0;
    if (!parse_int(buf, pos, crlf, num_elements) || num_elements < 0)
        return RespError::ProtocolError;

    pos = crlf + 2;

    RespCommand command;
    command.reserve(static_cast<size_t>(num_elements));

    for (int64_t i = 0; i < num_elements; ++i) {
        if (pos >= len) return RespError::NeedMoreData;
        if (buf[pos] != '$') return RespError::ProtocolError;
        ++pos;

        crlf = find_crlf(buf, len, pos);
        if (crlf == std::string::npos) return RespError::NeedMoreData;

        int64_t str_len = 0;
        if (!parse_int(buf, pos, crlf, str_len) || str_len < 0)
            return RespError::ProtocolError;

        pos = crlf + 2;

        if (pos + static_cast<size_t>(str_len) + 2 > len)
            return RespError::NeedMoreData;

        command.emplace_back(buf + pos, static_cast<size_t>(str_len));
        pos += static_cast<size_t>(str_len) + 2;
    }

    return ParseResult{std::move(command), pos};
}

std::string resp_simple_string(std::string_view s) {
    return "+" + std::string(s) + "\r\n";
}

std::string resp_error(std::string_view msg) {
    return "-" + std::string(msg) + "\r\n";
}

std::string resp_bulk_string(std::string_view s) {
    return "$" + std::to_string(s.size()) + "\r\n" + std::string(s) + "\r\n";
}

std::string resp_null_bulk() {
    return "$-1\r\n";
}

std::string resp_integer(int64_t n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string resp_array(const std::vector<std::string>& elements) {
    std::string out = "*" + std::to_string(elements.size()) + "\r\n";
    for (const auto& e : elements)
        out += resp_bulk_string(e);
    return out;
}

std::string resp_empty_array() {
    return "*0\r\n";
}
