#include "config.h"

#include <string>
#include <utility>
#include <vector>

#include <toml++/toml.h>

SharedConfig load_shared_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    SharedConfig config;
    if (auto net = root["network"].as_table()) {
        config.port = static_cast<uint16_t>(toml_get_int(*net, "port", config.port));
    }
    if (auto movement = root["movement"].as_table()) {
        config.move_step = toml_get_int(*movement, "move_step", config.move_step);
    }
    if (auto merchant = root["merchant"].as_table()) {
        config.merchant_sell_price_ratio =
                toml_get_double(*merchant, "sell_price_ratio", config.merchant_sell_price_ratio);
    }
    return config;
}

int toml_get_int(const toml::table& tbl, const char* key, int fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<int>()) {
            return *value;
        }
    }
    return fallback;
}

uint32_t toml_get_uint32(const toml::table& tbl, const char* key, uint32_t fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<int64_t>()) {
            if (*value >= 0) {
                return static_cast<uint32_t>(*value);
            }
        }
    }
    return fallback;
}

bool toml_get_bool(const toml::table& tbl, const char* key, bool fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<bool>()) {
            return *value;
        }
    }
    return fallback;
}

std::string toml_get_string(const toml::table& tbl, const char* key, const std::string& fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<std::string>()) {
            return *value;
        }
    }
    return fallback;
}

double toml_get_double(const toml::table& tbl, const char* key, double fallback) {
    if (auto node = tbl.get(key)) {
        if (auto value = node->value<double>()) {
            return *value;
        }
        if (auto value = node->value<int64_t>()) {
            return static_cast<double>(*value);
        }
    }
    return fallback;
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
    for (const auto& row_node: *rows) {
        const auto* row_array = row_node.as_array();
        if (!row_array) {
            continue;
        }
        std::vector<std::string> row;
        for (const auto& cell: *row_array) {
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

namespace {

void parse_tile_definitions(const toml::table& tiles, TilemapConfig& config) {
    for (const auto& [key, value]: tiles) {
        const auto* tile_tbl = value.as_table();
        if (!tile_tbl)
            continue;
        TileDef def;
        def.x = toml_get_int(*tile_tbl, "x", def.x);
        def.y = toml_get_int(*tile_tbl, "y", def.y);
        def.walkable = toml_get_bool(*tile_tbl, "walkable", def.walkable);
        def.path = toml_get_string(*tile_tbl, "path", std::string());
        config.tiles.emplace(key, def);
    }
}

void parse_mob_spawn_zones_data(const toml::table& zones, TilemapConfig& config) {
    config.mob_spawn_limit = toml_get_int(zones, "limit", config.mob_spawn_limit);
    config.mob_spawn_interval_ticks = toml_get_int(zones, "interval_ticks",
                                                   config.mob_spawn_interval_ticks);
    auto arr = zones["data"].as_array();
    if (!arr)
        return;
    for (const auto& row_node: *arr) {
        const auto* row_arr = row_node.as_array();
        if (!row_arr)
            continue;
        std::vector<bool> row;
        for (const auto& cell: *row_arr) {
            if (auto v = cell.value<bool>())
                row.push_back(*v);
        }
        if (!row.empty())
            config.mob_spawn_zones.push_back(std::move(row));
    }
}

}  // namespace

void parse_tilemap_config(const toml::table& root, TilemapConfig& config) {
    if (auto tilemap = root["tilemap"].as_table()) {
        config.path = toml_get_string(*tilemap, "path", std::string());
        config.tile_size = toml_get_int(*tilemap, "tile_size", config.tile_size);

        if (auto tiles = (*tilemap)["tiles"].as_table())
            parse_tile_definitions(*tiles, config);

        config.mapa = parse_map_grid(*tilemap);
    }

    if (auto metadata = root["metadata"].as_table()) {
        std::string type_str = toml_get_string(*metadata, "type", "");
        if (type_str == "city")
            config.map_type = MapType::CITY;
        else if (type_str == "dungeon")
            config.map_type = MapType::DUNGEON;
    }

    if (auto zones = root["mob_spawn_zones"].as_table())
        parse_mob_spawn_zones_data(*zones, config);
}

namespace {

void parse_prop_definitions(const toml::table& prop_tiles, TilemapConfig& config) {
    for (const auto& [key, value]: prop_tiles) {
        const auto* tbl = value.as_table();
        if (!tbl)
            continue;
        PropDef def;
        if (auto arr = (*tbl)["paths"].as_array()) {
            for (const auto& p: *arr) {
                if (auto s = p.value<std::string>())
                    def.paths.push_back(*s);
            }
        }
        if (auto src = (*tbl)["src"].as_table()) {
            def.src_x = toml_get_int(*src, "x", def.src_x);
            def.src_y = toml_get_int(*src, "y", def.src_y);
            def.src_w = toml_get_int(*src, "w", def.src_w);
            def.src_h = toml_get_int(*src, "h", def.src_h);
        }
        def.width = toml_get_int(*tbl, "width", def.width);
        def.height = toml_get_int(*tbl, "height", def.height);
        def.frame_ms = toml_get_uint32(*tbl, "frame_ms", def.frame_ms);
        if (auto hb = (*tbl)["hitbox"].as_table()) {
            def.hitbox.x = toml_get_int(*hb, "x", def.hitbox.x);
            def.hitbox.y = toml_get_int(*hb, "y", def.hitbox.y);
            def.hitbox.w = toml_get_int(*hb, "w", def.hitbox.w);
            def.hitbox.h = toml_get_int(*hb, "h", def.hitbox.h);
        }
        if (auto parts_arr = (*tbl)["parts"].as_array()) {
            for (const auto& part_node: *parts_arr) {
                const auto* part_tbl = part_node.as_table();
                if (!part_tbl)
                    continue;
                PropPartDef part;
                part.path = toml_get_string(*part_tbl, "path", std::string());
                part.src_x = toml_get_int(*part_tbl, "src_x", 0);
                part.src_y = toml_get_int(*part_tbl, "src_y", 0);
                part.src_w = toml_get_int(*part_tbl, "src_w", 0);
                part.src_h = toml_get_int(*part_tbl, "src_h", 0);
                part.offset_x = toml_get_int(*part_tbl, "offset_x", 0);
                part.offset_y = toml_get_int(*part_tbl, "offset_y", 0);
                def.parts.push_back(std::move(part));
            }
        }
        def.transition_map = toml_get_string(*tbl, "transition_map", std::string());
        def.transition_x = toml_get_int(*tbl, "transition_x", def.transition_x);
        def.transition_y = toml_get_int(*tbl, "transition_y", def.transition_y);
        config.props.emplace(key, def);
    }
}

void parse_prop_grid_data(const toml::table& pm, TilemapConfig& config) {
    if (auto grid = pm["data"].as_array()) {
        for (const auto& row_node: *grid) {
            const auto* row_array = row_node.as_array();
            if (!row_array)
                continue;
            std::vector<std::string> row;
            for (const auto& cell: *row_array) {
                if (auto value = cell.value<std::string>())
                    row.push_back(*value);
            }
            if (!row.empty())
                config.prop_map.push_back(std::move(row));
        }
    }

    auto overrides = pm["transition_overrides"].as_table();
    if (!overrides)
        return;
    for (const auto& [key, value]: *overrides) {
        const auto* tbl = value.as_table();
        if (!tbl) continue;

        std::string key_str{key.str()};
        auto comma = key_str.find(',');
        if (comma == std::string::npos) continue;

        int r = std::stoi(key_str.substr(0, comma));
        int c = std::stoi(key_str.substr(comma + 1));

        PropTransitionOverride ov;
        ov.transition_map = toml_get_string(*tbl, "transition_map", std::string());
        ov.transition_x = toml_get_int(*tbl, "transition_x", 0);
        ov.transition_y = toml_get_int(*tbl, "transition_y", 0);

        auto ru = static_cast<std::size_t>(r);
        auto cu = static_cast<std::size_t>(c);
        if (ru >= config.prop_transition_overrides.size())
            config.prop_transition_overrides.resize(ru + 1);
        if (cu >= config.prop_transition_overrides[ru].size())
            config.prop_transition_overrides[ru].resize(cu + 1, PropTransitionOverride{});
        config.prop_transition_overrides[ru][cu] = ov;
    }
}

}  // namespace

void parse_prop_config(const toml::table& root, TilemapConfig& config) {
    auto prop = root["prop"].as_table();
    if (!prop)
        return;

    if (auto prop_tiles = (*prop)["tiles"].as_table())
        parse_prop_definitions(*prop_tiles, config);

    if (auto pm = (*prop)["prop_map"].as_table())
        parse_prop_grid_data(*pm, config);
}

std::unordered_map<std::string, TilemapConfig> load_all_map_configs(
        const std::string& map_list_path) {
    toml::table root = toml::parse_file(map_list_path);
    std::unordered_map<std::string, TilemapConfig> result;

    if (auto maps = root["maps"].as_array()) {
        for (const auto& entry: *maps) {
            const auto* tbl = entry.as_table();
            if (!tbl)
                continue;

            std::string name = toml_get_string(*tbl, "name", std::string());
            std::string path = toml_get_string(*tbl, "path", std::string());
            if (name.empty() || path.empty())
                continue;

            toml::table map_root = toml::parse_file(path);
            TilemapConfig cfg;
            parse_tilemap_config(map_root, cfg);
            parse_prop_config(map_root, cfg);
            result.emplace(name, std::move(cfg));
        }
    }

    return result;
}
