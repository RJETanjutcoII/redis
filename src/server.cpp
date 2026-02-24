#include "server.h"
#include "resp.h"
#include "connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

Server::Server(Config cfg)
    : cfg_(std::move(cfg))
    , janitor_(store_, cfg_.janitor_interval_ms)
{
    if (cfg_.aof_enabled) {
        FsyncPolicy policy = FsyncPolicy::EverySecond;
        if (cfg_.aof_fsync == "always") policy = FsyncPolicy::Always;
        else if (cfg_.aof_fsync == "no")  policy = FsyncPolicy::No;
        aof_ = std::make_unique<AOFWriter>(cfg_.aof_path, policy);
    }
    setup_socket();
}

Server::~Server() {
    if (listen_fd_ != -1) close(listen_fd_);
}

void Server::setup_socket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg_.port);
    inet_pton(AF_INET, cfg_.bind_addr.c_str(), &addr.sin_addr);

    if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));

    if (listen(listen_fd_, 128) < 0)
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));

    std::cout << "Listening on " << cfg_.bind_addr << ":" << cfg_.port << "\n";
}

void Server::run() {
    while (true) {
        int client_fd = accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            std::cerr << "accept() failed: " << strerror(errno) << "\n";
            continue;
        }
        handle_client(client_fd);
        close(client_fd);
    }
}

void Server::handle_client(int fd) {
    CommandContext ctx{store_, aof_.get(), cfg_.server_version};
    std::string buf;
    char tmp[4096];

    while (true) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n <= 0) return;

        buf.append(tmp, n);

        while (true) {
            auto result = parse_resp(buf.data(), buf.size());

            if (std::holds_alternative<RespError>(result)) {
                auto err = std::get<RespError>(result);
                if (err == RespError::NeedMoreData) break;
                return;
            }

            auto& [cmd, consumed] = std::get<ParseResult>(result);
            buf.erase(0, consumed);

            std::string response = dispatcher_.dispatch(ctx, cmd);
            write(fd, response.data(), response.size());
        }
    }
}
