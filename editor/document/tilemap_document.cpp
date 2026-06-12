#include "tilemap_document.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../io/toml_serializer.h"

void TilemapDocument::load(const std::string& path) {
    config_ = TomlSerializer::load(path);
    if (config_.mapa.empty()) {
        throw std::runtime_error("Empty map grid in config file");
    }
    if (config_.prop_map.empty()) {
        config_.prop_map.resize(config_.mapa.size(),
                                std::vector<std::string>(config_.mapa[0].size(), ""));
    }
    if (config_.mob_spawn_zones.empty()) {
        config_.mob_spawn_zones.resize(config_.mapa.size(),
                                       std::vector<bool>(config_.mapa[0].size(), false));
    }
    path_ = path;
}

int TilemapDocument::width() const {
    if (config_.mapa.empty())
        return 0;
    return static_cast<int>(config_.mapa[0].size());
}

int TilemapDocument::height() const { return static_cast<int>(config_.mapa.size()); }

int TilemapDocument::tile_size() const { return config_.tile_size; }

const std::string& TilemapDocument::tile_name(int row, int col) const {
    return config_.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void TilemapDocument::set_tile(int row, int col, const std::string& name) {
    config_.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = name;
}

const std::string& TilemapDocument::prop_name(int row, int col) const {
    return config_.prop_map[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void TilemapDocument::set_prop(int row, int col, const std::string& name) {
    config_.prop_map[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = name;
}

bool TilemapDocument::is_mob_spawn_zone(int row, int col) const {
    return config_.mob_spawn_zones[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void TilemapDocument::set_tile_walkable(const std::string& name, bool walkable) {
    auto it = config_.tiles.find(name);
    if (it != config_.tiles.end())
        it->second.walkable = walkable;
}

PropTransitionOverride TilemapDocument::transition_override(int row, int col) const {
    auto r = static_cast<std::size_t>(row);
    auto c = static_cast<std::size_t>(col);
    if (r < config_.prop_transition_overrides.size() &&
        c < config_.prop_transition_overrides[r].size()) {
        return config_.prop_transition_overrides[r][c];
    }
    return {};
}

void TilemapDocument::set_transition_override(int row, int col,
                                              const PropTransitionOverride& override) {
    auto r = static_cast<std::size_t>(row);
    auto c = static_cast<std::size_t>(col);
    if (r >= config_.prop_transition_overrides.size())
        config_.prop_transition_overrides.resize(r + 1);
    if (c >= config_.prop_transition_overrides[r].size())
        config_.prop_transition_overrides[r].resize(
                c + 1, PropTransitionOverride{});
    config_.prop_transition_overrides[r][c] = override;
}

void TilemapDocument::set_mob_spawn_zone(int row, int col, bool value) {
    config_.mob_spawn_zones[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = value;
}

void TilemapDocument::resize(int new_rows, int new_cols, const std::string& default_tile) {
    resize_grid(config_.mapa, new_rows, new_cols, default_tile);
    resize_grid(config_.prop_map, new_rows, new_cols, std::string{});
    resize_grid(config_.mob_spawn_zones, new_rows, new_cols, false);
    resize_grid(config_.prop_transition_overrides, new_rows, new_cols, PropTransitionOverride{});
}

void TilemapDocument::create_new(int rows, int cols, const TilemapConfig& tile_config,
                                 MapType map_type) {
    config_ = {};
    config_.path = tile_config.path;
    config_.tile_size = tile_config.tile_size;
    config_.tiles = tile_config.tiles;
    config_.props = tile_config.props;
    config_.map_type = map_type;
    resize_grid(config_.mapa, rows, cols, std::string{});
    resize_grid(config_.prop_map, rows, cols, std::string{});
    resize_grid(config_.mob_spawn_zones, rows, cols, false);
    resize_grid(config_.prop_transition_overrides, rows, cols, PropTransitionOverride{});
    path_.clear();
}

void TilemapDocument::save(const std::string& path) const {
    TomlSerializer::save(path, config_);
}
