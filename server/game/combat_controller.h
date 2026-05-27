#ifndef COMBAT_CONTROLLER_H
#define COMBAT_CONTROLLER_H

#include <cstdint>
#include <map>

#include "../../common/messages.h"
#include "../core/config.h"

#include "../../common/messages.h"
#include "../core/config.h"

#include "command_result.h"
#include "player.h"

class ClanManager;

class CombatController {
public:
    static constexpr int CLAN_BONUS_RANGE_PX = 200;
    static constexpr double CLAN_BONUS_PER_MEMBER = 0.05;
    static constexpr double CLAN_BONUS_MAX = 0.25;

    CombatController(const AttackConfig& config, std::map<uint16_t, Player>& players);

    void set_clan_manager(ClanManager& mgr);

    CommandResult melee_attack(uint16_t attacker_id, uint16_t target_id, uint32_t current_tick);

private:
    bool in_range(const Player& attacker, const Player& target) const;
    uint32_t calculate_damage() const;
    int count_nearby_clan_members(const Player& player) const;

    const AttackConfig& config;
    std::map<uint16_t, Player>& players;
    ClanManager* clan_manager = nullptr;
};

#endif
