#pragma once
#include "config.h"
#include "store.h"
#include "command.h"
#include "expiry.h"
#include "aof.h"
#include <memory>

class Server {
public:
    explicit Server(Config cfg);
    ~Server();
    void run();

private:
    void setup_socket();
    void handle_client(int fd);

    Config cfg_;
    KeyValueStore store_;
    CommandDispatcher dispatcher_;
    ExpiryJanitor janitor_;
    std::unique_ptr<AOFWriter> aof_;
    int listen_fd_ = -1;
};
