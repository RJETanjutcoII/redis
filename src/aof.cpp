#include "aof.h"

AOFWriter::AOFWriter(std::string_view path, FsyncPolicy policy)
    : path_(path), policy_(policy) {}

AOFWriter::~AOFWriter() {}
void AOFWriter::append(const std::vector<std::string>&) {}
void AOFWriter::flush() {}
bool AOFWriter::is_open() const { return false; }
void AOFWriter::fsync_if_needed() {}

AOFReplayer::AOFReplayer(std::string_view path) : path_(path) {}
std::vector<std::vector<std::string>> AOFReplayer::replay() { return {}; }
std::string AOFReplayer::error() const { return error_; }
