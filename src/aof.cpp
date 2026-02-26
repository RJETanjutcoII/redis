#include "aof.h"
#include "resp.h"

AOFWriter::AOFWriter(std::string_view path, FsyncPolicy policy)
    : path_(path), policy_(policy), last_fsync_(std::chrono::steady_clock::now()) {
    file_.open(path_, std::ios::app | std::ios::binary);
}

AOFWriter::~AOFWriter() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

bool AOFWriter::is_open() const {
    return file_.is_open();
}

void AOFWriter::append(const std::vector<std::string>& command) {
    if (!file_.is_open()) return;
    file_ << resp_array(command);
    fsync_if_needed();
}

void AOFWriter::flush() {
    if (!file_.is_open()) return;
    file_.flush();
}

void AOFWriter::fsync_if_needed() {
    if (policy_ == FsyncPolicy::Always) {
        file_.flush();
    } else if (policy_ == FsyncPolicy::EverySecond) {
        auto now = std::chrono::steady_clock::now();
        if (now - last_fsync_ >= std::chrono::seconds(1)) {
            file_.flush();
                last_fsync_ = now;
        }
    }
}

AOFReplayer::AOFReplayer(std::string_view path) : path_(path) {}

std::vector<std::vector<std::string>> AOFReplayer::replay() {
    std::ifstream file(path_.c_str(), std::ios::binary);
    if (!file.is_open()) {
        error_ = "could not open AOF file: " + path_;
        return {};
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    std::vector<std::vector<std::string>> commands;
    size_t pos = 0;
    while (pos < content.size()) {
        auto result = parse_resp(content.c_str() + pos, content.size() - pos);
        if (std::holds_alternative<RespError>(result)) {
            auto err = std::get<RespError>(result);
            if (err == RespError::NeedMoreData) break;
            error_ = "AOF parse error at offset " + std::to_string(pos);
            break;
        }
        auto [cmd, consumed] = std::get<ParseResult>(result);
        commands.push_back(cmd);
        pos += consumed;
    }
    return commands;
}

std::string AOFReplayer::error() const {
    return error_;
}
