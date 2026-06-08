#ifndef COMBAT_CONTROLLER_H
#define COMBAT_CONTROLLER_H

#include <cstdint>
#include <map>
#include <string>

#include "../../common/messages.h"
#include "../core/config.h"

#include "command_result.h"
#include "enemy_npc.h"
#include "player.h"

class ClanManager;

class CombatController {
public:
    CombatController(const AttackConfig& config, std::map<uint16_t, Player>& players,
                     const ItemCatalog& catalog, std::map<uint16_t, EnemyNpc>& enemy_npcs);

    void set_clan_manager(ClanManager& mgr);
    CommandResult melee_attack(uint16_t attacker_id, uint16_t target_id, uint32_t current_tick);
    CommandResult update_npc_ai(uint32_t current_tick);

private:
    bool in_range(uint16_t attacker_x, uint16_t attacker_y, uint16_t target_x,
                  uint16_t target_y) const;
    uint32_t calculate_damage(const Player& attacker);
    int count_nearby_clan_members(const Player& player) const;
    double get_clan_damage_bonus(const Player& attacker) const;
    CommandResult notify_entity_attacked(Player& attacker, uint16_t target_id, uint32_t damage,
                                         uint32_t target_hp_current, uint32_t target_hp_max,
                                         const std::string& target_name,
                                         const std::string& target_clan_name, bool target_is_dead,
                                         uint8_t target_level);
    bool is_critical_attack(const Player& attacker);
    uint32_t calculate_defense(const Player& target);
    uint32_t calculate_object_defense(const InventorySlot& object_slot);
    Player& get_nearest_player(uint16_t enemy_x, uint16_t enemy_y);
    CommandResult melee_attack_player(uint16_t attacker_id, uint16_t target_id,
                                      uint32_t current_tick);
    CommandResult melee_attack_npc(uint16_t attacker_id, uint16_t npc_target_id,
                                   uint32_t current_tick);

    const AttackConfig& config;
    std::map<uint16_t, Player>& players;
    ClanManager* clan_manager = nullptr;
    const ItemCatalog& item_catalog_;
    Rng rng;
    std::map<uint16_t, EnemyNpc>& enemy_npcs;
};

#endif
