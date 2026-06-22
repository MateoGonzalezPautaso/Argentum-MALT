#include "toml_serializer.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

TilemapConfig TomlSerializer::load(const std::string& path) {
    toml::table root = toml::parse_file(path);
    TilemapConfig config;
    parse_tilemap_config(root, config);
    parse_prop_config(root, config);
    return config;
}

toml::table TomlSerializer::save_tilemap_table(const TilemapConfig& config) {
    toml::table tilemap_tbl;

    tilemap_tbl.emplace("path", config.path);
    tilemap_tbl.emplace("tile_size", config.tile_size);

    toml::array mapa_array;
    for (const auto& row: config.mapa) {
        toml::array row_array;
        std::for_each(row.begin(), row.end(),
                      [&row_array](const auto& v) { row_array.push_back(v); });
        mapa_array.push_back(std::move(row_array));
    }
    tilemap_tbl.emplace("mapa", std::move(mapa_array));

    toml::table tiles_tbl;
    for (const auto& [name, def]: config.tiles) {
        toml::table tile_def;
        tile_def.emplace("x", def.x);
        tile_def.emplace("y", def.y);
        if (!def.walkable) {
            tile_def.emplace("walkable", false);
        }
        if (!def.path.empty()) {
            tile_def.emplace("path", def.path);
        }
        tiles_tbl.emplace(name, std::move(tile_def));
    }
    tilemap_tbl.emplace("tiles", std::move(tiles_tbl));

    return tilemap_tbl;
}

toml::table TomlSerializer::save_metadata(const TilemapConfig& config) {
    if (config.map_type == MapType::NONE)
        return {};

    toml::table metadata;
    std::string type_str = config.map_type == MapType::CITY ? "city" : "dungeon";
    metadata.emplace("type", type_str);
    return metadata;
}

toml::table TomlSerializer::save_mob_spawn_zones(const TilemapConfig& config) {
    auto row_has_zone = [](const auto& row) {
        return std::any_of(row.begin(), row.end(), [](bool v) { return v; });
    };
    bool has_zones =
            std::any_of(config.mob_spawn_zones.begin(), config.mob_spawn_zones.end(), row_has_zone);
    if (!has_zones)
        return {};

    toml::array zones_grid;
    for (const auto& row: config.mob_spawn_zones) {
        toml::array row_array;
        for (bool v: row) {
            row_array.push_back(v);
        }
        zones_grid.push_back(std::move(row_array));
    }
    toml::table zones_tbl;
    if (config.mob_spawn_limit > 0)
        zones_tbl.emplace("limit", config.mob_spawn_limit);
    if (config.mob_spawn_interval_ticks > 0)
        zones_tbl.emplace("interval_ticks", config.mob_spawn_interval_ticks);
    zones_tbl.emplace("data", std::move(zones_grid));
    return zones_tbl;
}

toml::table TomlSerializer::save_props(const TilemapConfig& config) {
    if (config.props.empty())
        return {};

    toml::table prop_tbl;

    toml::table prop_tiles_tbl;
    for (const auto& [name, def]: config.props) {
        toml::table prop_def;

        if (!def.paths.empty()) {
            toml::array paths_arr;
            std::for_each(def.paths.begin(), def.paths.end(),
                          [&paths_arr](const auto& v) { paths_arr.push_back(v); });
            prop_def.emplace("paths", std::move(paths_arr));

            toml::table src;
            src.emplace("x", def.src_x);
            src.emplace("y", def.src_y);
            src.emplace("w", def.src_w);
            src.emplace("h", def.src_h);
            prop_def.emplace("src", std::move(src));
        }

        prop_def.emplace("width", def.width);
        prop_def.emplace("height", def.height);
        if (def.frame_ms > 0) {
            prop_def.emplace("frame_ms", static_cast<int64_t>(def.frame_ms));
        }
        if (def.hitbox.w > 0 && def.hitbox.h > 0) {
            toml::table hb;
            hb.emplace("x", def.hitbox.x);
            hb.emplace("y", def.hitbox.y);
            hb.emplace("w", def.hitbox.w);
            hb.emplace("h", def.hitbox.h);
            prop_def.emplace("hitbox", std::move(hb));
        }
        if (!def.transition_map.empty()) {
            prop_def.emplace("transition_map", def.transition_map);
            prop_def.emplace("transition_x", def.transition_x);
            prop_def.emplace("transition_y", def.transition_y);
        }

        if (!def.parts.empty()) {
            toml::array parts_arr;
            for (const auto& part: def.parts) {
                toml::table part_def;
                part_def.emplace("path", part.path);
                part_def.emplace("src_x", part.src_x);
                part_def.emplace("src_y", part.src_y);
                part_def.emplace("src_w", part.src_w);
                part_def.emplace("src_h", part.src_h);
                part_def.emplace("offset_x", part.offset_x);
                part_def.emplace("offset_y", part.offset_y);
                parts_arr.push_back(std::move(part_def));
            }
            prop_def.emplace("parts", std::move(parts_arr));
        }
        prop_tiles_tbl.emplace(name, std::move(prop_def));
    }
    prop_tbl.emplace("tiles", std::move(prop_tiles_tbl));

    auto row_has_prop = [](const auto& row) {
        return std::any_of(row.begin(), row.end(), [](const auto& cell) { return !cell.empty(); });
    };
    bool has_props = std::any_of(config.prop_map.begin(), config.prop_map.end(), row_has_prop);

    if (has_props) {
        toml::array prop_grid;
        for (const auto& row: config.prop_map) {
            toml::array row_array;
            std::for_each(row.begin(), row.end(),
                          [&row_array](const auto& v) { row_array.push_back(v); });
            prop_grid.push_back(std::move(row_array));
        }
        bool has_overrides = false;
        toml::table override_tbl;
        for (std::size_t r = 0; r < config.prop_transition_overrides.size(); ++r) {
            for (std::size_t c = 0; c < config.prop_transition_overrides[r].size(); ++c) {
                const auto& ov = config.prop_transition_overrides[r][c];
                if (ov.transition_map.empty())
                    continue;
                toml::table ov_entry;
                ov_entry.emplace("transition_map", ov.transition_map);
                if (ov.transition_x != 0 || ov.transition_y != 0) {
                    ov_entry.emplace("transition_x", ov.transition_x);
                    ov_entry.emplace("transition_y", ov.transition_y);
                }
                std::string key = std::to_string(r) + "," + std::to_string(c);
                override_tbl.emplace(key, std::move(ov_entry));
                has_overrides = true;
            }
        }

        toml::table pm;
        pm.emplace("data", std::move(prop_grid));
        if (has_overrides)
            pm.emplace("transition_overrides", std::move(override_tbl));
        prop_tbl.emplace("prop_map", std::move(pm));
    }

    return prop_tbl;
}

void TomlSerializer::save(const std::string& path, const TilemapConfig& config) {
    toml::table root;
    root.emplace("tilemap", save_tilemap_table(config));

    auto metadata = save_metadata(config);
    if (!metadata.empty())
        root.emplace("metadata", std::move(metadata));

    auto zones = save_mob_spawn_zones(config);
    if (!zones.empty())
        root.emplace("mob_spawn_zones", std::move(zones));

    auto props = save_props(config);
    if (!props.empty())
        root.emplace("prop", std::move(props));

    std::ofstream file(path);
    file << root << std::endl;
}
