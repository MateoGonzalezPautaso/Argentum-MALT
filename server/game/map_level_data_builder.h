#ifndef SERVER_MAP_LEVEL_DATA_BUILDER_H
#define SERVER_MAP_LEVEL_DATA_BUILDER_H

#include <string>

#include "../../common/messages.h"

#include "map.h"

/*
 * MapLevelDataBuilder
 *
 * Responsabilidad única: transformar el estado en memoria que el servidor ya
 * tiene (TilemapConfig + PropGrid, vía Map) en un MapLevelData listo para
 * transmitir por red.
 *
 * No introduce lógica de colisión nueva: reutiliza Map::is_walkable() para
 * exponer la walkability que el servidor ya calcula celda a celda.
 */
class MapLevelDataBuilder {
public:
    static MapLevelData build(const std::string& map_name, const Map& map);
};

#endif  // SERVER_MAP_LEVEL_DATA_BUILDER_H
