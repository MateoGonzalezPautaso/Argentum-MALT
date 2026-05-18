#include "config.h"

#include <stdexcept>
#include <string>

#include <toml++/toml.h>

#include "../common/config.h"

ServerConfig load_server_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ServerConfig config;

    if (auto server = root["server"].as_table()) {
        config.port =
                static_cast<uint16_t>(toml_get_int(*server, "port",
                                                   static_cast<int>(config.port)));
    }

    parse_tilemap_config(root, config.tilemap);

    if (config.tilemap.mapa.empty()) {
        throw std::runtime_error("server config requires a tilemap with 'mapa'");
    }

    if (auto movement = root["movement"].as_table()) {
        config.move_step = toml_get_int(*movement, "move_step", config.move_step);
    }

    if (auto sprite = root["sprite"].as_table()) {
        config.sprite_width = toml_get_int(*sprite, "width", config.sprite_width);
        config.sprite_height = toml_get_int(*sprite, "height", config.sprite_height);
    }

    return config;
}
