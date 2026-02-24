#pragma once
#include <string>

enum class ConnectionState {
    Reading,
    Writing,
    Closing,
};

struct Connection {
    int fd;
    ConnectionState state = ConnectionState::Reading;
    std::string read_buf;
    std::string write_buf;

    void enqueue_response(std::string data) {
        write_buf += std::move(data);
    }
};
