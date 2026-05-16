#include "config.h"

#include <stdexcept>

#include <toml++/toml.h>

namespace {
int get_int(const toml::table& tbl, const char* key, int fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<int>()) {
            return *value;
        }
    }
    return fallback;
}

uint32_t get_uint32(const toml::table& tbl, const char* key, uint32_t fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<int64_t>()) {
            if (*value >= 0) {
                return static_cast<uint32_t>(*value);
            }
        }
    }
    return fallback;
}

bool get_bool(const toml::table& tbl, const char* key, bool fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<bool>()) {
            return *value;
        }
    }
    return fallback;
}

std::string get_string(const toml::table& tbl, const char* key, const std::string& fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<std::string>()) {
            return *value;
        }
    }
    return fallback;
}

std::vector<std::string> parse_paths(const toml::table& tbl) {
    std::vector<std::string> paths;

    if (auto path_value = tbl["path"].value<std::string>()) {
        paths.push_back(*path_value);
    }

    if (auto paths_array = tbl["paths"].as_array()) {
        for (const auto& item : *paths_array) {
            if (auto value = item.value<std::string>()) {
                paths.push_back(*value);
            }
        }
    }

    return paths;
}

std::vector<std::vector<std::string>> parse_map_grid(const toml::table& tbl) {
    std::vector<std::vector<std::string>> grid;
    const toml::array* rows = tbl["mapa"].as_array();
    if (!rows) {
        rows = tbl["map"].as_array();
    }
    if (!rows) {
        return grid;
    }
    for (const auto& row_node : *rows) {
        const auto* row_array = row_node.as_array();
        if (!row_array) {
            continue;
        }
        std::vector<std::string> row;
        for (const auto& cell : *row_array) {
            if (auto value = cell.value<std::string>()) {
                row.push_back(*value);
            }
        }
        if (!row.empty()) {
            grid.push_back(std::move(row));
        }
    }
    return grid;
}

void parse_window_config(const toml::table& root, ClientConfig& config) {
    if (auto window = root["window"].as_table()) {
        config.window.width = get_int(*window, "width", config.window.width);
        config.window.height = get_int(*window, "height", config.window.height);
        config.window.title = get_string(*window, "title", config.window.title);
    }
}

void parse_background_config(const toml::table& root, ClientConfig& config) {
    if (auto background = root["background"].as_table()) {
        config.background.path = get_string(*background, "path", std::string());
        config.background.x = get_int(*background, "x", config.background.x);
        config.background.y = get_int(*background, "y", config.background.y);
        config.background.width = get_int(*background, "width", config.window.width);
        config.background.height = get_int(*background, "height", config.window.height);
    }

    if (config.background.path.empty()) {
        throw std::runtime_error("background.path is required in config");
    }
}

void parse_tilemap_config(const toml::table& root, ClientConfig& config) {
    if (auto tilemap = root["tilemap"].as_table()) {
        config.tilemap.path = get_string(*tilemap, "path", std::string());
        config.tilemap.tile_size = get_int(*tilemap, "tile_size", config.tilemap.tile_size);

        if (auto tiles = (*tilemap)["tiles"].as_table()) {
            for (const auto& [key, value] : *tiles) {
                const auto* tile_tbl = value.as_table();
                if (!tile_tbl) {
                    continue;
                }
                TileDef def;
                def.x = get_int(*tile_tbl, "x", def.x);
                def.y = get_int(*tile_tbl, "y", def.y);
                config.tilemap.tiles.emplace(key, def);
            }
        }

        config.tilemap.mapa = parse_map_grid(*tilemap);
    }
}

void parse_sprite_configs(const toml::table& root, ClientConfig& config) {
    if (auto sprites = root["sprites"].as_array()) {
        for (const auto& entry : *sprites) {
            if (!entry.is_table()) {
                continue;
            }
            const toml::table& sprite_tbl = *entry.as_table();
            SpriteConfig sprite;
            sprite.paths = parse_paths(sprite_tbl);
            sprite.x = get_int(sprite_tbl, "x", sprite.x);
            sprite.y = get_int(sprite_tbl, "y", sprite.y);
            sprite.width = get_int(sprite_tbl, "width", sprite.width);
            sprite.height = get_int(sprite_tbl, "height", sprite.height);
            sprite.src_x = get_int(sprite_tbl, "src_x", sprite.src_x);
            sprite.src_y = get_int(sprite_tbl, "src_y", sprite.src_y);
            sprite.src_width = get_int(sprite_tbl, "src_width", sprite.src_width);
            sprite.src_height = get_int(sprite_tbl, "src_height", sprite.src_height);
            sprite.frame_ms = get_uint32(sprite_tbl, "frame_ms", sprite.frame_ms);
            sprite.movable = get_bool(sprite_tbl, "movable", sprite.movable);
            sprite.anchor_to_movable = get_bool(sprite_tbl, "anchor_to_movable", sprite.anchor_to_movable);
            sprite.anchor_offset_x = get_int(sprite_tbl, "anchor_offset_x", sprite.anchor_offset_x);
            sprite.anchor_offset_y = get_int(sprite_tbl, "anchor_offset_y", sprite.anchor_offset_y);
            sprite.visible = get_bool(sprite_tbl, "visible", sprite.visible);

            if (!sprite.paths.empty() && sprite.frame_ms == 0 && sprite.paths.size() > 1) {
                sprite.frame_ms = 120;
            }

            if (!sprite.paths.empty()) {
                config.sprites.push_back(sprite);
            }
        }
    }

    if (config.sprites.empty()) {
        throw std::runtime_error("config needs at least one sprite entry");
    }
}

void parse_movement_config(const toml::table& root, ClientConfig& config) {
    if (auto movement = root["movement"].as_table()) {
        config.move_step = get_int(*movement, "move_step", config.move_step);
        config.walk_src_step = get_int(*movement, "walk_src_step", config.walk_src_step);
        config.walk_src_frames = get_int(*movement, "walk_src_frames", config.walk_src_frames);
        config.walk_src_frames_down = get_int(*movement, "walk_src_frames_down", config.walk_src_frames);
        config.walk_src_frames_up = get_int(*movement, "walk_src_frames_up", config.walk_src_frames);
        config.walk_src_frames_left = get_int(*movement, "walk_src_frames_left", config.walk_src_frames);
        config.walk_src_frames_right = get_int(*movement, "walk_src_frames_right", config.walk_src_frames);
        config.walk_frame_ms = get_uint32(*movement, "walk_frame_ms", config.walk_frame_ms);
        config.tick_ms = get_uint32(*movement, "tick_ms", config.tick_ms);
        config.dir_src_y_down = get_int(*movement, "dir_src_y_down", config.dir_src_y_down);
        config.dir_src_y_up = get_int(*movement, "dir_src_y_up", config.dir_src_y_up);
        config.dir_src_y_left = get_int(*movement, "dir_src_y_left", config.dir_src_y_left);
        config.dir_src_y_right = get_int(*movement, "dir_src_y_right", config.dir_src_y_right);
        config.head_dir_src_y_down = get_int(*movement, "head_dir_src_y_down", config.head_dir_src_y_down);
        config.head_dir_src_y_up = get_int(*movement, "head_dir_src_y_up", config.head_dir_src_y_up);
        config.head_dir_src_y_left = get_int(*movement, "head_dir_src_y_left", config.head_dir_src_y_left);
        config.head_dir_src_y_right = get_int(*movement, "head_dir_src_y_right", config.head_dir_src_y_right);
    }
}
}

ClientConfig load_client_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ClientConfig config;

    parse_window_config(root, config);
    parse_background_config(root, config);
    parse_tilemap_config(root, config);
    parse_sprite_configs(root, config);
    parse_movement_config(root, config);

    return config;
}
