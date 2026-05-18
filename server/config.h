#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <cstdint>
#include <string>

#include "../common/config.h"

struct ServerConfig {
    uint16_t port = 1234;
    TilemapConfig tilemap;
    int move_step = 4;
    int sprite_width = 27;
    int sprite_height = 48;
};

ServerConfig load_server_config(const std::string& path);

#endif  // SERVER_CONFIG_H
