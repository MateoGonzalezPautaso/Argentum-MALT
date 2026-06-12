#ifndef EDITOR_TOML_SERIALIZER_H
#define EDITOR_TOML_SERIALIZER_H

#include <string>

#include <toml++/toml.h>

#include "common/config.h"

class TomlSerializer {
public:
    static TilemapConfig load(const std::string& path);
    static void save(const std::string& path, const TilemapConfig& config);

private:
    static toml::table save_tilemap_table(const TilemapConfig& config);
    static toml::table save_metadata(const TilemapConfig& config);
    static toml::table save_mob_spawn_zones(const TilemapConfig& config);
    static toml::table save_props(const TilemapConfig& config);
};

#endif
