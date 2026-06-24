#include "movement_service.h"

#include <cmath>
#include <cstdlib>

#include "../../../common/error_logger.h"
#include "../../../common/messages.h"

namespace {

struct Delta {
    int dx;
    int dy;
};

Delta direction_to_delta(Direction dir, int step) {
    switch (dir) {
        case Direction::NORTH:
            return {0, -step};
        case Direction::SOUTH:
            return {0, step};
        case Direction::WEST:
            return {-step, 0};
        case Direction::EAST:
            return {step, 0};
    }
    return {0, 0};
}

}  // namespace

MovementService::MovementService(
        std::map<uint16_t, Player>& players, std::unordered_map<std::string, Map>& maps,
        const std::map<uint16_t, EnemyNpc>& enemy_npcs,
        const std::unordered_map<uint16_t, PendingResurrection>& pending_resurrections,
        const BalanceConfig& balance, int move_step, int sprite_width, int sprite_height,
        MapTransitionService& map_transition_service):
        players_(players),
        maps_(maps),
        enemy_npcs_(enemy_npcs),
        pending_resurrections_(pending_resurrections),
        balance_(balance),
        move_step_(move_step),
        sprite_width_(sprite_width),
        sprite_height_(sprite_height),
        map_transition_service_(map_transition_service) {}

Map& MovementService::player_map(const Player& p) {
    auto it = maps_.find(p.get_current_map());
    if (it == maps_.end()) {
        ErrorLogger::log("[MovementService] player_map: map '" + p.get_current_map() +
                         "' not found for player '" + p.get_name() + "' (id=" +
                         std::to_string(p.get_id()) + "), falling back to an arbitrary map");
        return maps_.begin()->second;
    }
    return it->second;
}

bool MovementService::collides_with_entities(uint16_t moving_player_id, const std::string& map_name,
                                             int current_x, int current_y, int new_x,
                                             int new_y) const {
    const int hw = sprite_width_ / 2;
    const int hh = sprite_height_ / 2;

    for (const auto& [other_id, other]: players_) {
        if (other_id == moving_player_id)
            continue;
        if (other.get_current_map() != map_name)
            continue;
        const int ox = static_cast<int>(other.pos_x());
        const int oy = static_cast<int>(other.pos_y());
        bool already_overlapping = (std::abs(current_x - ox) < hw && std::abs(current_y - oy) < hh);
        if (already_overlapping)
            continue;
        if (std::abs(new_x - ox) < hw && std::abs(new_y - oy) < hh)
            return true;
    }

    for (const auto& [npc_id, npc]: enemy_npcs_) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != map_name)
            continue;
        const int nx = static_cast<int>(npc.pos_x());
        const int ny = static_cast<int>(npc.pos_y());
        bool already_overlapping = (std::abs(current_x - nx) < hw && std::abs(current_y - ny) < hh);
        if (already_overlapping)
            continue;
        if (std::abs(new_x - nx) < hw && std::abs(new_y - ny) < hh)
            return true;
    }

    return false;
}

CommandResult MovementService::handle_move(uint16_t player_id, const MoveCmd& cmd) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};

    Player& player = it->second;
    player.set_meditating(false);

    if (pending_resurrections_.contains(player_id))
        return {};

    Map& cur_map = player_map(player);

    int effective_step = player.has_cheat_fast_velocity() ?
                                 move_step_ * balance_.cheat_velocity_multiplier :
                                 move_step_;
    auto [dx, dy] = direction_to_delta(cmd.direction, effective_step);

    const int current_x = static_cast<int>(player.pos_x());
    const int current_y = static_cast<int>(player.pos_y());
    const int new_x = cur_map.clamp_x(current_x + dx, sprite_width_);
    const int new_y = cur_map.clamp_y(current_y + dy, sprite_height_);

    if (!cur_map.is_walkable(new_x + sprite_width_ / 2, new_y + sprite_height_))
        return {};

    if (collides_with_entities(player_id, player.get_current_map(), current_x, current_y, new_x,
                               new_y))
        return {};

    const int final_dx = new_x - current_x;
    const int final_dy = new_y - current_y;
    if (final_dx == 0 && final_dy == 0)
        return {};

    player.apply_move(cmd.direction, final_dx, final_dy);

    CommandResult result;

    if (map_transition_service_.try_map_transition(player, result))
        return result;

    EntityMoveEvent move{
            .entity_id = player.get_id(),
            .entity_pos = player.get_pos(),
            .entity_dir = player.get_dir(),
    };
    result.map_events = {move};
    return result;
}
