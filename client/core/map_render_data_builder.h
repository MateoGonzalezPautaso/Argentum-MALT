#ifndef CLIENT_MAP_RENDER_DATA_BUILDER_H
#define CLIENT_MAP_RENDER_DATA_BUILDER_H

#include "../../common/config.h"
#include "../../common/messages.h"
#include "../config/map_visuals_config.h"

/*
 * MapRenderDataBuilder
 *
 * Responsabilidad única: combinar el MapLevelData que llegó por red (estructura
 * del nivel) con el MapVisualCatalog local (cómo se dibuja cada tile/prop) para
 * reconstruir un TilemapConfig — exactamente el tipo que ya consumen
 * TilemapRenderer y PropRenderer. Así no hace falta tocar los renderers: el
 * cliente sigue dibujando como siempre, pero la geometría ahora viene del
 * servidor en vez de leerse de disco.
 */
class MapRenderDataBuilder {
public:
    static TilemapConfig build(const MapLevelData& level, const MapVisualCatalog& visuals);
};

#endif  // CLIENT_MAP_RENDER_DATA_BUILDER_H
