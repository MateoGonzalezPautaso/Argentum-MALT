#include "combat_controller.h"

#include <cstdlib>
#include <algorithm>

CombatController::CombatController(const AttackConfig& config,
                                   std::map<uint16_t, Player>& players):
        config(config),
        players(players) {}

CommandResult CombatController::melee_attack(uint16_t attacker_id, uint16_t target_id,
                                              uint32_t current_tick) {
    auto attacker_it = players.find(attacker_id);
    if (attacker_it == players.end())
        return {};

    auto target_it = players.find(target_id);
    if (target_it == players.end())
        return {};

    if (attacker_id == target_id)
        return {};

    Player& attacker = attacker_it->second;
    Player& target = target_it->second;

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    if (!in_range(attacker, target))
        return {};

    uint32_t damage = calculate_damage();
    target.take_damage(damage);

    DamageDealtEvent dealt{target_id, damage};
    DamageReceivedEvent received{attacker_id, damage};
    std::vector<ServerEvent> broadcast = {received};

    if (target.hp_current == 0) {
        EntityDiedEvent died{target_id};
        broadcast.push_back(died);
    }

    return {
            .private_events = {dealt},
            .broadcast_events = broadcast,
    };
}

bool CombatController::in_range(const Player& attacker, const Player& target) const {
    const int dx = static_cast<int>(target.pos_x()) - static_cast<int>(attacker.pos_x());
    const int dy = static_cast<int>(target.pos_y()) - static_cast<int>(attacker.pos_y());
    const int dist_sq = dx * dx + dy * dy;
    const int range_sq = config.attack_range_px * config.attack_range_px;
    return dist_sq <= range_sq;
}

uint32_t CombatController::calculate_damage() const {
    int variance = 0;
    if (config.damage_variance > 0)
        variance = (std::rand() % (2 * config.damage_variance + 1)) - config.damage_variance;
    return static_cast<uint32_t>(std::max(1, config.base_damage + variance));
}
