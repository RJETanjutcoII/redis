#include "server.h"
#include "resp.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <cerrno>

static const int MAX_EVENTS = 64;

Server::Server(Config cfg)
    : cfg_(std::move(cfg))
    , janitor_(store_, cfg_.janitor_interval_ms)
{
    if (cfg_.aof_enabled) {
        FsyncPolicy policy = FsyncPolicy::EverySecond;
        if (cfg_.aof_fsync == "always") policy = FsyncPolicy::Always;
        else if (cfg_.aof_fsync == "no") policy = FsyncPolicy::No;

        AOFReplayer replayer(cfg_.aof_path);
        auto commands = replayer.replay();
        if (!replayer.error().empty())
            std::cerr << "AOF replay warning: " << replayer.error() << "\n";
        CommandContext ctx{store_, nullptr, "0.1.0"};
        for (auto& cmd : commands)
            dispatcher_.dispatch(ctx, cmd);

        aof_ = std::make_unique<AOFWriter>(cfg_.aof_path, policy);
    }
    setup_socket();
    setup_epoll();
}

Server::~Server() {
    if (epoll_fd_ != -1) close(epoll_fd_);
    if (listen_fd_ != -1) close(listen_fd_);
}

void Server::setup_socket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // reclaim port immediately on restart

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg_.port);
    inet_pton(AF_INET, cfg_.bind_addr.c_str(), &addr.sin_addr);

    if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));

    if (listen(listen_fd_, 128) < 0)
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));

    int flags = fcntl(listen_fd_, F_GETFL, 0);
    fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK); // required for edge-triggered epoll

    std::cout << "Listening on " << cfg_.bind_addr << ":" << cfg_.port << "\n";
}

void Server::setup_epoll() {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0)
        throw std::runtime_error("epoll_create1() failed: " + std::string(strerror(errno)));

    epoll_event ev{};
    // EPOLLET: edge-triggered — we are responsible for draining until EAGAIN.
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
}

void Server::run() {
    janitor_.start();
    epoll_event events[MAX_EVENTS];

    while (true) {
        // Drain janitor deletions before blocking — janitor never touches data_ directly.
        auto deletions = janitor_.drain_deletions();
        store_.delete_expired_batch(deletions);

        int nfds;
        do {
            nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, 100);
        } while (nfds < 0 && errno == EINTR);

        if (nfds < 0) {
            std::cerr << "epoll_wait() failed: " << strerror(errno) << "\n";
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listen_fd_) {
                handle_accept();
            } else if (ev & (EPOLLHUP | EPOLLERR)) {
                close_connection(fd);
            } else {
                if (ev & EPOLLIN) handle_read(fd);
                if (ev & EPOLLOUT) handle_write(fd);
            }
        }
    }
}

void Server::handle_accept() {
    while (true) {
        // accept4 sets NONBLOCK and CLOEXEC atomically — avoids a race between accept+fcntl.
        int client_fd = accept4(listen_fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            std::cerr << "accept4() failed: " << strerror(errno) << "\n";
            break;
        }

        connections_.emplace(client_fd, Connection{client_fd});

        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);
    }
}

void Server::handle_read(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) return;
    Connection& conn = it->second;

    char tmp[4096];
    while (true) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            close_connection(fd);
            return;
        }
        if (n == 0) { // peer closed connection
            close_connection(fd);
            return;
        }
        conn.read_buf.append(tmp, n);
    }

    CommandContext ctx{store_, aof_.get(), cfg_.server_version};

    while (true) {
        auto result = parse_resp(conn.read_buf.data(), conn.read_buf.size());

        if (std::holds_alternative<RespError>(result)) {
            if (std::get<RespError>(result) == RespError::NeedMoreData) break;
            close_connection(fd);
            return;
        }

        auto& [cmd, consumed] = std::get<ParseResult>(result);
        conn.read_buf.erase(0, consumed);
        conn.enqueue_response(dispatcher_.dispatch(ctx, cmd));
    }

    if (!conn.write_buf.empty()) // switch to watching EPOLLOUT so we know when to flush
        rearm_epoll(fd, EPOLLIN | EPOLLOUT | EPOLLET);
}

void Server::handle_write(int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) return;
    Connection& conn = it->second;

    while (!conn.write_buf.empty()) {
        ssize_t n = write(fd, conn.write_buf.data(), conn.write_buf.size());
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            close_connection(fd);
            return;
        }
        conn.write_buf.erase(0, n);
    }

    if (conn.write_buf.empty()) // fully drained — stop watching EPOLLOUT
        rearm_epoll(fd, EPOLLIN | EPOLLET);
}

void Server::close_connection(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    connections_.erase(fd);
}

void Server::rearm_epoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}
