#include "map_render_data_builder.h"

#include <cstddef>
#include <string>
#include <vector>

TilemapConfig MapRenderDataBuilder::build(const MapLevelData& level,
                                          const MapVisualCatalog& visuals) {
    TilemapConfig cfg;
    cfg.path = visuals.tilemap_path;
    cfg.tile_size = level.tile_size;
    cfg.map_type = level.map_type;

    // Las definiciones visuales (cómo dibujar cada tile/prop) son locales.
    cfg.tiles = visuals.tile_visuals;
    cfg.props = visuals.prop_visuals;

    // Reconstruir la grilla de tiles resolviendo cada índice contra el diccionario.
    cfg.mapa.resize(level.tile_grid.size());
    for (std::size_t row = 0; row < level.tile_grid.size(); ++row) {
        cfg.mapa[row].reserve(level.tile_grid[row].size());
        for (uint16_t index: level.tile_grid[row]) {
            const std::string& id =
                    index < level.tile_id_table.size() ? level.tile_id_table[index] : std::string();
            cfg.mapa[row].push_back(id);
        }
    }

    // Reconstruir la grilla densa de props a partir de la lista dispersa.
    cfg.prop_map.assign(level.rows, std::vector<std::string>(level.cols, std::string()));
    for (const PropPlacement& p: level.props) {
        if (p.prop_id_index >= level.prop_id_table.size())
            continue;
        if (p.row >= cfg.prop_map.size() || p.col >= cfg.prop_map[p.row].size())
            continue;
        cfg.prop_map[p.row][p.col] = level.prop_id_table[p.prop_id_index];
    }

    cfg.mob_spawn_zones = level.mob_spawn_zones;

    return cfg;
}
