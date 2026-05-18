#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <toml++/toml.h>

struct TileDef {
    int x = 0;
    int y = 0;
    bool walkable = true;
};

struct TilemapConfig {
    std::string path;
    int tile_size = 128;
    std::unordered_map<std::string, TileDef> tiles;
    std::vector<std::vector<std::string>> mapa;
};

int toml_get_int(const toml::table& tbl, const char* key, int fallback);
uint32_t toml_get_uint32(const toml::table& tbl, const char* key, uint32_t fallback);
bool toml_get_bool(const toml::table& tbl, const char* key, bool fallback);
std::string toml_get_string(const toml::table& tbl, const char* key,
                            const std::string& fallback);

std::vector<std::vector<std::string>> parse_map_grid(const toml::table& tbl);
void parse_tilemap_config(const toml::table& root, TilemapConfig& config);

#endif  // COMMON_CONFIG_H
