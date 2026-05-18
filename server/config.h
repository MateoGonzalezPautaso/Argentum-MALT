#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <cstdint>
#include <string>

#include "../common/config.h"

struct BalanceConfig {
    int starting_hp = 100;
    int starting_mana = 50;
    int starting_gold = 0;
    int max_level = 100;
    int hp_per_level = 10;
    int mana_per_level = 5;
    int gold_per_level = 100;
    int level_exp_base = 1000;
    double level_exp_exponent = 1.8;
    int gold_cap_base = 100;
    double gold_cap_exponent = 1.1;
};

struct ServerConfig {
    uint16_t port = 1234;
    TilemapConfig tilemap;
    int move_step = 4;
    int sprite_width = 27;
    int sprite_height = 48;
    BalanceConfig balance;
};

ServerConfig load_server_config(const std::string& path);

#endif  // SERVER_CONFIG_H
