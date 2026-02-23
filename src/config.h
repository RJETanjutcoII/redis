#pragma once
#include <cstdint>
#include <string>

struct Config {
    uint16_t    port                = 6379;
    std::string bind_addr           = "127.0.0.1";
    std::string aof_path            = "appendonly.aof";
    bool        aof_enabled         = false;
    std::string aof_fsync           = "everysecond"; // always | everysecond | no
    int         janitor_interval_ms = 100;
    std::string server_version      = "0.1.0";
};

Config parse_args(int argc, char** argv);
