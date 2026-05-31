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
    std::string path;
};

struct HitboxDef {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct PropPartDef {
    std::string path;
    int src_x = 0;
    int src_y = 0;
    int src_w = 0;
    int src_h = 0;
    int offset_x = 0;
    int offset_y = 0;
};

struct PropDef {
    std::vector<std::string> paths;
    int src_x = 0;
    int src_y = 0;
    int src_w = 0;
    int src_h = 0;
    int width = 0;
    int height = 0;
    uint32_t frame_ms = 0;
    HitboxDef hitbox;
    std::vector<PropPartDef> parts;
};

struct TilemapConfig {
    std::string path;
    int tile_size = 128;
    std::unordered_map<std::string, TileDef> tiles;
    std::vector<std::vector<std::string>> mapa;
    std::unordered_map<std::string, PropDef> props;
    std::vector<std::vector<std::string>> prop_map;
};

int toml_get_int(const toml::table& tbl, const char* key, int fallback);
uint32_t toml_get_uint32(const toml::table& tbl, const char* key, uint32_t fallback);
bool toml_get_bool(const toml::table& tbl, const char* key, bool fallback);
std::string toml_get_string(const toml::table& tbl, const char* key, const std::string& fallback);
double toml_get_double(const toml::table& tbl, const char* key, double fallback);

std::vector<std::vector<std::string>> parse_map_grid(const toml::table& tbl);
void parse_tilemap_config(const toml::table& root, TilemapConfig& config);
void parse_prop_config(const toml::table& root, TilemapConfig& config);

#endif  // COMMON_CONFIG_H
