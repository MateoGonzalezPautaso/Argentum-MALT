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
}

ClientConfig load_client_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ClientConfig config;

    if (auto window = root["window"].as_table()) {
        config.window.width = get_int(*window, "width", config.window.width);
        config.window.height = get_int(*window, "height", config.window.height);
        config.window.title = get_string(*window, "title", config.window.title);
    }

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
            sprite.visible = get_bool(sprite_tbl, "visible", sprite.visible);

            if (!sprite.paths.empty() && sprite.frame_ms == 0 && sprite.paths.size() > 1) {
                sprite.frame_ms = 120;
            }

            if (!sprite.paths.empty()) {
                config.sprites.push_back(sprite);
            }
        }
    }

    if (auto movement = root["movement"].as_table()) {
        config.move_step = get_int(*movement, "move_step", config.move_step);
        config.walk_src_step = get_int(*movement, "walk_src_step", config.walk_src_step);
        config.walk_src_max = get_int(*movement, "walk_src_max", config.walk_src_max);
        config.walk_frame_ms = get_uint32(*movement, "walk_frame_ms", config.walk_frame_ms);
        config.tick_ms = get_uint32(*movement, "tick_ms", config.tick_ms);
        config.dir_src_y_down = get_int(*movement, "dir_src_y_down", config.dir_src_y_down);
        config.dir_src_y_up = get_int(*movement, "dir_src_y_up", config.dir_src_y_up);
        config.dir_src_y_left = get_int(*movement, "dir_src_y_left", config.dir_src_y_left);
        config.dir_src_y_right = get_int(*movement, "dir_src_y_right", config.dir_src_y_right);
    }

    if (config.sprites.empty()) {
        throw std::runtime_error("config needs at least one sprite entry");
    }

    return config;
}
