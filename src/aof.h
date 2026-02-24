#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <chrono>

enum class FsyncPolicy {
    Always,
    EverySecond,
    No,
};

class AOFWriter {
public:
    explicit AOFWriter(std::string_view path, FsyncPolicy policy = FsyncPolicy::EverySecond);
    ~AOFWriter();

    void append(const std::vector<std::string>& command);
    void flush();
    bool is_open() const;

private:
    void fsync_if_needed();

    std::ofstream file_;
    std::string path_;
    FsyncPolicy policy_;
    std::chrono::steady_clock::time_point last_fsync_;
};

class AOFReplayer {
public:
    explicit AOFReplayer(std::string_view path);
    std::vector<std::vector<std::string>> replay();
    std::string error() const;

private:
    std::string path_;
    std::string error_;
};
