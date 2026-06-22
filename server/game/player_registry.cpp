#include "player_registry.h"

PlayerRegistry::PlayerRegistry(const std::map<uint16_t, Player>& players): players_(players) {}

std::vector<uint16_t> PlayerRegistry::ids_on_map(const std::string& map_name) const {
    std::vector<uint16_t> ids;
    for (const auto& [id, player]: players_) {
        if (player.get_current_map() == map_name)
            ids.push_back(id);
    }
    return ids;
}
