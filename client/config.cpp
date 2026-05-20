#include "config.h"

#include <stdexcept>
#include <utility>

#include <toml++/toml.h>

namespace {

std::vector<std::string> parse_paths(const toml::table& tbl) {
    std::vector<std::string> paths;

    if (auto path_value = tbl["path"].value<std::string>()) {
        paths.push_back(*path_value);
    }

    if (auto paths_array = tbl["paths"].as_array()) {
        for (const auto& item: *paths_array) {
            if (auto value = item.value<std::string>()) {
                paths.push_back(*value);
            }
        }
    }

    return paths;
}

void parse_window_config(const toml::table& root, ClientConfig& config) {
    if (auto window = root["window"].as_table()) {
        config.window.width = toml_get_int(*window, "width", config.window.width);
        config.window.height = toml_get_int(*window, "height", config.window.height);
        config.window.title = toml_get_string(*window, "title", config.window.title);
    }
}

void parse_background_config(const toml::table& root, ClientConfig& config) {
    if (auto background = root["background"].as_table()) {
        config.background.path = toml_get_string(*background, "path", std::string());
        config.background.x = toml_get_int(*background, "x", config.background.x);
        config.background.y = toml_get_int(*background, "y", config.background.y);
        config.background.width = toml_get_int(*background, "width", config.window.width);
        config.background.height = toml_get_int(*background, "height", config.window.height);
    }

    if (config.background.path.empty()) {
        throw std::runtime_error("background.path is required in config");
    }
}

void parse_sprite_configs(const toml::table& root, ClientConfig& config) {
    if (auto sprites = root["sprites"].as_array()) {
        for (const auto& entry: *sprites) {
            if (!entry.is_table()) {
                continue;
            }
            const toml::table& sprite_tbl = *entry.as_table();
            SpriteConfig sprite;
            sprite.paths = parse_paths(sprite_tbl);
            sprite.x = toml_get_int(sprite_tbl, "x", sprite.x);
            sprite.y = toml_get_int(sprite_tbl, "y", sprite.y);
            sprite.width = toml_get_int(sprite_tbl, "width", sprite.width);
            sprite.height = toml_get_int(sprite_tbl, "height", sprite.height);
            sprite.src_x = toml_get_int(sprite_tbl, "src_x", sprite.src_x);
            sprite.src_y = toml_get_int(sprite_tbl, "src_y", sprite.src_y);
            sprite.src_width = toml_get_int(sprite_tbl, "src_width", sprite.src_width);
            sprite.src_height = toml_get_int(sprite_tbl, "src_height", sprite.src_height);
            sprite.frame_ms = toml_get_uint32(sprite_tbl, "frame_ms", sprite.frame_ms);
            sprite.movable = toml_get_bool(sprite_tbl, "movable", sprite.movable);
            sprite.anchor_to_movable =
                    toml_get_bool(sprite_tbl, "anchor_to_movable", sprite.anchor_to_movable);
            sprite.anchor_offset_x =
                    toml_get_int(sprite_tbl, "anchor_offset_x", sprite.anchor_offset_x);
            sprite.anchor_offset_y =
                    toml_get_int(sprite_tbl, "anchor_offset_y", sprite.anchor_offset_y);
            sprite.visible = toml_get_bool(sprite_tbl, "visible", sprite.visible);

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
        config.move_step = toml_get_int(*movement, "move_step", config.move_step);
        config.walk_src_step = toml_get_int(*movement, "walk_src_step", config.walk_src_step);
        config.walk_src_frames = toml_get_int(*movement, "walk_src_frames", config.walk_src_frames);
        config.walk_src_frames_down =
                toml_get_int(*movement, "walk_src_frames_down", config.walk_src_frames);
        config.walk_src_frames_up =
                toml_get_int(*movement, "walk_src_frames_up", config.walk_src_frames);
        config.walk_src_frames_left =
                toml_get_int(*movement, "walk_src_frames_left", config.walk_src_frames);
        config.walk_src_frames_right =
                toml_get_int(*movement, "walk_src_frames_right", config.walk_src_frames);
        config.walk_frame_ms = toml_get_uint32(*movement, "walk_frame_ms", config.walk_frame_ms);
        config.tick_ms = toml_get_uint32(*movement, "tick_ms", config.tick_ms);
        config.dir_src_y_down = toml_get_int(*movement, "dir_src_y_down", config.dir_src_y_down);
        config.dir_src_y_up = toml_get_int(*movement, "dir_src_y_up", config.dir_src_y_up);
        config.dir_src_y_left = toml_get_int(*movement, "dir_src_y_left", config.dir_src_y_left);
        config.dir_src_y_right = toml_get_int(*movement, "dir_src_y_right", config.dir_src_y_right);
        config.head_dir_src_y_down =
                toml_get_int(*movement, "head_dir_src_y_down", config.head_dir_src_y_down);
        config.head_dir_src_y_up =
                toml_get_int(*movement, "head_dir_src_y_up", config.head_dir_src_y_up);
        config.head_dir_src_y_left =
                toml_get_int(*movement, "head_dir_src_y_left", config.head_dir_src_y_left);
        config.head_dir_src_y_right =
                toml_get_int(*movement, "head_dir_src_y_right", config.head_dir_src_y_right);
    }
}
}  // namespace

ClientConfig load_client_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ClientConfig config;

    parse_window_config(root, config);
    parse_background_config(root, config);
    parse_sprite_configs(root, config);
    parse_movement_config(root, config);

    {
        toml::table tilemap_tbl = toml::parse_file("config/common_tilemap.toml");
        parse_tilemap_config(tilemap_tbl, config.tilemap);
        parse_prop_config(tilemap_tbl, config.tilemap);
    }

    return config;
}
