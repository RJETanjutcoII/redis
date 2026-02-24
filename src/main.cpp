#include "config.h"
#include "server.h"
#include <iostream>
#include <stdexcept>
#include <cstring>

Config parse_args(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            cfg.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        else if (strcmp(argv[i], "--bind") == 0 && i + 1 < argc)
            cfg.bind_addr = argv[++i];
        else if (strcmp(argv[i], "--aof-enabled") == 0)
            cfg.aof_enabled = true;
        else if (strcmp(argv[i], "--aof-path") == 0 && i + 1 < argc)
            cfg.aof_path = argv[++i];
        else if (strcmp(argv[i], "--aof-fsync") == 0 && i + 1 < argc)
            cfg.aof_fsync = argv[++i];
    }
    return cfg;
}

int main(int argc, char** argv) {
    Config cfg = parse_args(argc, argv);

    try {
        Server server(cfg);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
