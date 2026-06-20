#ifndef COMBAT_CONTROLLER_H
#define COMBAT_CONTROLLER_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "../../common/messages.h"
#include "../core/config.h"

#include "command_result.h"
#include "enemy_npc.h"
#include "map.h"
#include "player.h"

class ClanManager;

class CombatController {
public:
    CombatController(const AttackConfig& config, std::map<uint16_t, Player>& players,
                     const ItemCatalog& catalog, std::map<uint16_t, EnemyNpc>& enemy_npcs,
                     const NpcDropConfig& drop_config);

    void set_clan_manager(ClanManager& mgr);
    CommandResult melee_attack(uint16_t attacker_id, uint16_t target_id, uint32_t current_tick);
    CommandResult update_npc_ai(uint32_t current_tick);

    void set_maps(const std::unordered_map<std::string, Map>& m) { maps = &m; }

    CommandResult spell_attack_player(uint16_t attacker_id, uint16_t target_id,
                                      uint32_t current_tick);
    CommandResult spell_attack_npc(uint16_t attacker_id, uint16_t npc_target_id,
                                    uint32_t current_tick);

    uint8_t crit_chance_for(const Player& p) const;
    uint8_t dodge_chance_for(const Player& p) const;
    std::pair<uint16_t, uint16_t> unarmed_damage_range() const;

    void fill_player_stats_event(PlayerStatsEvent& ev, const Player& p) const;

private:
    bool in_range(uint16_t attacker_x, uint16_t attacker_y, uint16_t target_x,
                  uint16_t target_y) const;
    bool in_range(uint16_t attacker_x, uint16_t attacker_y, uint16_t target_x, uint16_t target_y,
                  uint32_t range_px) const;
    uint32_t calculate_damage(const Player& attacker);
    int count_nearby_clan_members(const Player& player) const;
    double get_clan_bonus(const Player& player) const;
    CommandResult notify_entity_attacked(Player& attacker, uint16_t target_id, uint32_t damage,
                                         uint32_t target_hp_current, uint32_t target_hp_max,
                                         const std::string& target_name,
                                         const std::string& target_clan_name, bool target_is_dead,
                                         uint8_t target_level, bool esquivado);
    bool is_critical_attack(const Player& attacker);
    uint32_t calculate_defense(const Player& target);
    uint32_t calculate_object_defense(const InventorySlot& object_slot);
    Player* get_nearest_player(const EnemyNpc& npc);
    void drop_inventory_on_death(Player& target,
                                 std::map<std::string, std::vector<ItemDroppedEvent>>& drops,
                                 std::vector<ServerEvent>& target_events);
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
    NpcDropConfig npc_drop_config;
    const std::unordered_map<std::string, Map>* maps = nullptr;
};

#endif
