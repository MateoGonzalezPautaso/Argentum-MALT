#ifndef COMBAT_CONTROLLER_H
#define COMBAT_CONTROLLER_H

#include <cstdint>
#include <map>

#include "../../common/messages.h"

#include "command_result.h"
#include "../core/config.h"
#include "player.h"

class CombatController {
public:
    CombatController(const AttackConfig& config, std::map<uint16_t, Player>& players);

    CommandResult melee_attack(uint16_t attacker_id, uint16_t target_id,
                               uint32_t current_tick);

private:
    bool in_range(const Player& attacker, const Player& target) const;
    uint32_t calculate_damage() const;

    const AttackConfig& config;
    std::map<uint16_t, Player>& players;
};

#endif
