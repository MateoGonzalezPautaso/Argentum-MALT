#ifndef GROUND_ITEM_SERVICE_H
#define GROUND_ITEM_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../../common/item_catalog.h"
#include "../../../common/messages.h"
#include "../command_result.h"
#include "../map.h"
#include "../player.h"

class GroundItemService {
public:
    GroundItemService(std::map<uint16_t, Player>& players,
                      std::unordered_map<std::string, Map>& maps, const ItemCatalog& item_catalog);

    CommandResult handle_pickup_item(uint16_t player_id, const PickupItemCmd& cmd);
    CommandResult handle_drop_item(uint16_t player_id, const DropItemCmd& cmd);

    // Materializa drops en el suelo y notifica a los jugadores en el mapa.
    // La firma es idéntica a la que tenía Game para no romper los callsites.
    void commit_ground_drops(CommandResult& result,
                             const std::map<std::string, std::vector<ItemDroppedEvent>>& drops);

    std::vector<ServerEvent> make_existing_ground_items(const std::string& map_name) const;

    static std::pair<int, int> tile_cell(const Map& map, int px, int py);

private:
    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;

    // mapa -> celda (tile_x, tile_y) -> lista de items tirados en el piso
    std::map<std::string, std::map<std::pair<int, int>, std::vector<ItemDroppedEvent>>>
            ground_items_;
    std::map<uint16_t, Player>& players_;
    std::unordered_map<std::string, Map>& maps_;
    const ItemCatalog& item_catalog_;
};

#endif  // GROUND_ITEM_SERVICE_H
