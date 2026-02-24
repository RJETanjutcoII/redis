#pragma once
#include "config.h"
#include "store.h"
#include "command.h"
#include "expiry.h"
#include "aof.h"
#include "connection.h"
#include <memory>
#include <unordered_map>

class Server {
public:
    explicit Server(Config cfg);
    ~Server();
    void run();

private:
    void setup_socket();
    void setup_epoll();
    void handle_accept();
    void handle_read(int fd);
    void handle_write(int fd);
    void close_connection(int fd);
    void rearm_epoll(int fd, uint32_t events);

    Config cfg_;
    KeyValueStore store_;
    CommandDispatcher dispatcher_;
    ExpiryJanitor janitor_;
    std::unique_ptr<AOFWriter> aof_;
    int listen_fd_ = -1;
    int epoll_fd_ = -1;
    std::unordered_map<int, Connection> connections_;
};
