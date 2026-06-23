#ifndef CLIENT_MAP_VISUALS_CONFIG_H
#define CLIENT_MAP_VISUALS_CONFIG_H

#include <string>
#include <unordered_map>

#include "../../common/config.h"

/*
 * MapVisualCatalog
 *
 * Catálogo *visual* local de un mapa: lo que el cliente necesita para DIBUJAR
 * cada tile y prop (paths de assets, frames, tamaños, offsets de hitbox). Es
 * contenido pre-instalado del cliente, análogo a los skins de NPC en client.toml.
 *
 * NO contiene la geometría del nivel (mapa, prop_map, walkable, spawn zones): eso
 * es dato de gameplay que el cliente descarga del servidor (MapLevelData). La
 * combinación de ambos la hace MapRenderDataBuilder.
 *
 * Se carga desde config/visuals/<map_name>.toml, que reutiliza el mismo formato
 * de [tilemap.tiles] y [prop.tiles] que los mapas originales.
 */
struct MapVisualCatalog {
    std::string tilemap_path;  // atlas global de fallback ([tilemap].path)
    std::unordered_map<std::string, TileDef> tile_visuals;
    std::unordered_map<std::string, PropDef> prop_visuals;
};

// Carga un único catálogo visual desde un archivo TOML.
MapVisualCatalog load_map_visual_catalog(const std::string& path);

// Carga todos los catálogos visuales del directorio dado (config/visuals/),
// usando el nombre de archivo (sin extensión) como nombre de mapa.
std::unordered_map<std::string, MapVisualCatalog> load_all_map_visual_catalogs(
        const std::string& visuals_dir);

#endif  // CLIENT_MAP_VISUALS_CONFIG_H
