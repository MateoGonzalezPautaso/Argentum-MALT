#ifndef RESURRECTION_SERVICE_H
#define RESURRECTION_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../enemy_npc.h"
#include "../map.h"
#include "../pending_resurrection.h"
#include "../player.h"
#include "ground_item_service.h"

class ResurrectionService {
public:
    ResurrectionService(std::map<uint16_t, Player>& players,
                        std::unordered_map<std::string, Map>& maps,
                        std::map<uint16_t, EnemyNpc>& enemy_npcs,
                        std::unordered_map<uint16_t, PendingResurrection>& pending_resurrections,
                        GroundItemService& ground_item_service,
                        const BalanceConfig& balance,
                        const MessagesConfig& msgs,
                        int sprite_width,
                        int sprite_height,
                        int tick_rate_hz);

    CommandResult handle_resurrect(uint16_t player_id);
    CommandResult process_pending_resurrections();

private:
    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;
    void append_existing_entities(std::vector<ServerEvent>& events, uint16_t exclude_id,
                                  const std::string& map_name) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id,
                                                  const std::string& map_name) const;

    std::map<uint16_t, Player>& players_;
    std::unordered_map<std::string, Map>& maps_;
    std::map<uint16_t, EnemyNpc>& enemy_npcs_;
    std::unordered_map<uint16_t, PendingResurrection>& pending_resurrections_;
    GroundItemService& ground_item_service_;
    const BalanceConfig& balance_;
    const MessagesConfig& msgs_;
    int sprite_width_;
    int sprite_height_;
    int tick_rate_hz_;
};

#endif  // RESURRECTION_SERVICE_H
