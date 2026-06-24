#ifndef MOVEMENT_SERVICE_H
#define MOVEMENT_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../enemy_npc.h"
#include "../map.h"
#include "../pending_resurrection.h"
#include "../player.h"

#include "map_transition_service.h"

class MovementService {
public:
    MovementService(std::map<uint16_t, Player>& players, std::unordered_map<std::string, Map>& maps,
                    const std::map<uint16_t, EnemyNpc>& enemy_npcs,
                    const std::unordered_map<uint16_t, PendingResurrection>& pending_resurrections,
                    const BalanceConfig& balance, int move_step, int sprite_width,
                    int sprite_height, MapTransitionService& map_transition_service);

    CommandResult handle_move(uint16_t player_id, const MoveCmd& cmd);

private:
    bool collides_with_entities(uint16_t moving_player_id, const std::string& map_name,
                                int current_x, int current_y, int new_x, int new_y) const;

    Map& player_map(const Player& p);

    std::map<uint16_t, Player>& players_;
    std::unordered_map<std::string, Map>& maps_;
    const std::map<uint16_t, EnemyNpc>& enemy_npcs_;
    const std::unordered_map<uint16_t, PendingResurrection>& pending_resurrections_;
    const BalanceConfig& balance_;
    int move_step_;
    int sprite_width_;
    int sprite_height_;
    MapTransitionService& map_transition_service_;
};

#endif  // MOVEMENT_SERVICE_H
