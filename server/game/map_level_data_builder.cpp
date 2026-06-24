#include "map_level_data_builder.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace {

// Devuelve el índice del string `id` dentro de `table`, agregándolo si es nuevo.
// cada id único aparece una sola vez en la tabla y las celdas guardan solo el índice.
uint16_t intern(const std::string& id, std::vector<std::string>& table,
                std::unordered_map<std::string, uint16_t>& lookup) {
    auto it = lookup.find(id);
    if (it != lookup.end())
        return it->second;
    auto index = static_cast<uint16_t>(table.size());
    table.push_back(id);
    lookup.emplace(id, index);
    return index;
}

bool placement_is_transition(const TilemapConfig& cfg, const std::string& prop_name,
                             std::size_t row, std::size_t col) {
    auto def_it = cfg.props.find(prop_name);
    if (def_it != cfg.props.end() && !def_it->second.transition_map.empty())
        return true;
    if (row < cfg.prop_transition_overrides.size() &&
        col < cfg.prop_transition_overrides[row].size())
        return !cfg.prop_transition_overrides[row][col].transition_map.empty();
    return false;
}

// Reutiliza Map::is_walkable() para exponer celda a celda la walkability que el
// servidor ya calcula; no introduce lógica de colisión nueva.
std::vector<std::vector<bool>> compute_walkable_grid(const Map& map) {
    const TilemapConfig& cfg = map.config();
    const int tile_size = cfg.tile_size;
    std::vector<std::vector<bool>> walkable(cfg.mapa.size());

    for (std::size_t row = 0; row < cfg.mapa.size(); ++row) {
        walkable[row].resize(cfg.mapa[row].size());
        for (std::size_t col = 0; col < cfg.mapa[row].size(); ++col) {
            // Centro de la celda en píxeles: misma convención que usa el servidor
            // para spawns.
            const int foot_x = static_cast<int>(col) * tile_size + tile_size / 2;
            const int foot_y = static_cast<int>(row) * tile_size + tile_size / 2;
            walkable[row][col] = map.is_walkable(foot_x, foot_y);
        }
    }
    return walkable;
}

}  // namespace

MapLevelData MapLevelDataBuilder::build(const std::string& map_name, const Map& map) {
    const TilemapConfig& cfg = map.config();

    MapLevelData data;
    data.map_name = map_name;
    data.map_type = cfg.map_type;
    data.tile_size = static_cast<uint16_t>(cfg.tile_size);
    data.rows = static_cast<uint16_t>(cfg.mapa.size());
    data.cols = static_cast<uint16_t>(cfg.mapa.empty() ? 0 : cfg.mapa.front().size());

    // Grilla de tiles -> diccionario + índices.
    std::unordered_map<std::string, uint16_t> tile_lookup;
    data.tile_grid.resize(cfg.mapa.size());
    for (std::size_t row = 0; row < cfg.mapa.size(); ++row) {
        data.tile_grid[row].reserve(cfg.mapa[row].size());
        for (const auto& tile_id: cfg.mapa[row]) {
            data.tile_grid[row].push_back(intern(tile_id, data.tile_id_table, tile_lookup));
        }
    }

    // Props -> diccionario + lista dispersa (solo celdas con prop).
    std::unordered_map<std::string, uint16_t> prop_lookup;
    for (std::size_t row = 0; row < cfg.prop_map.size(); ++row) {
        for (std::size_t col = 0; col < cfg.prop_map[row].size(); ++col) {
            const std::string& name = cfg.prop_map[row][col];
            if (name.empty())
                continue;
            PropPlacement placement;
            placement.prop_id_index = intern(name, data.prop_id_table, prop_lookup);
            placement.row = static_cast<uint16_t>(row);
            placement.col = static_cast<uint16_t>(col);
            placement.is_transition = placement_is_transition(cfg, name, row, col);
            data.props.push_back(placement);
        }
    }

    data.walkable = compute_walkable_grid(map);
    data.mob_spawn_zones = cfg.mob_spawn_zones;

    return data;
}
