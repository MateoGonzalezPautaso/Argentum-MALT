#include "toml_serializer.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

TilemapConfig TomlSerializer::load(const std::string& path) {
    toml::table root = toml::parse_file(path);
    TilemapConfig config;
    parse_tilemap_config(root, config);
    parse_prop_config(root, config);
    return config;
}

void TomlSerializer::save(const std::string& path, const TilemapConfig& config) {
    toml::table tilemap_tbl;

    tilemap_tbl.emplace("path", config.path);
    tilemap_tbl.emplace("tile_size", config.tile_size);

    toml::array mapa_array;
    for (const auto& row : config.mapa) {
        toml::array row_array;
        std::for_each(row.begin(), row.end(),
                      [&row_array](const auto& v) { row_array.push_back(v); });
        mapa_array.push_back(std::move(row_array));
    }
    tilemap_tbl.emplace("mapa", std::move(mapa_array));

    toml::table tiles_tbl;
    for (const auto& [name, def] : config.tiles) {
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

    toml::table root;
    root.emplace("tilemap", std::move(tilemap_tbl));

    if (config.map_type != MapType::NONE) {
        toml::table metadata;
        std::string type_str = config.map_type == MapType::CITY ? "city" : "dungeon";
        metadata.emplace("type", type_str);
        root.emplace("metadata", std::move(metadata));
    }

    auto row_has_zone = [](const auto& row) {
        return std::any_of(row.begin(), row.end(), [](bool v) { return v; });
    };
    bool has_zones =
            std::any_of(config.mob_spawn_zones.begin(), config.mob_spawn_zones.end(), row_has_zone);
    if (has_zones) {
        toml::array zones_grid;
        for (const auto& row : config.mob_spawn_zones) {
            toml::array row_array;
            for (bool v : row) {
                row_array.push_back(v);
            }
            zones_grid.push_back(std::move(row_array));
        }
        toml::table zones_tbl;
        zones_tbl.emplace("data", std::move(zones_grid));
        root.emplace("mob_spawn_zones", std::move(zones_tbl));
    }

    if (!config.props.empty()) {
        toml::table prop_tbl;

        toml::table prop_tiles_tbl;
        for (const auto& [name, def] : config.props) {
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
            if (!def.parts.empty()) {
                toml::array parts_arr;
                for (const auto& part : def.parts) {
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
            return std::any_of(row.begin(), row.end(),
                               [](const auto& cell) { return !cell.empty(); });
        };
        bool has_props =
                std::any_of(config.prop_map.begin(), config.prop_map.end(), row_has_prop);

        if (has_props) {
            toml::array prop_grid;
            for (const auto& row : config.prop_map) {
                toml::array row_array;
                std::for_each(row.begin(), row.end(),
                              [&row_array](const auto& v) { row_array.push_back(v); });
                prop_grid.push_back(std::move(row_array));
            }
            toml::table pm;
            pm.emplace("data", std::move(prop_grid));
            prop_tbl.emplace("prop_map", std::move(pm));
        }

        root.emplace("prop", std::move(prop_tbl));
    }

    std::ofstream file(path);
    file << root << std::endl;
}
