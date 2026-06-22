#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../../common/messages.h"
#include "../../common/rng.h"
#include "../core/config.h"
#include "../persistence/clan_persistence.h"
#include "services/bank_service.h"
#include "services/cheat_service.h"
#include "services/ground_item_service.h"
#include "services/map_transition_service.h"
#include "services/merchant_service.h"
#include "services/player_session_service.h"
#include "services/spawn_service.h"

#include "clan_command_handler.h"
#include "clan_manager.h"
#include "combat_controller.h"
#include "command_result.h"
#include "enemy_npc.h"
#include "map.h"
#include "player.h"
#include "player_data_service.h"

class Game {
private:
    std::map<uint16_t, Player> players;
    std::unordered_map<std::string, uint16_t> player_name_index_;
    PlayerDataService& player_data_service;
    ClanManager clan_manager;
    ClanCommandHandler clan_handler;
    std::unordered_map<std::string, TilemapConfig> tilemap_configs;
    std::unordered_map<std::string, Map> maps;
    int move_step;
    int sprite_width;
    int sprite_height;
    BalanceConfig balance;
    InventoryConfig inventory_config;
    MobSpawnConfig mob_spawn;
    const ItemCatalog& item_catalog;
    Rng rng;
    std::map<uint16_t, EnemyNpc> enemy_npcs;
    std::vector<NpcTemplate> world_npc_templates;
    std::vector<NpcTemplate> dungeon_npc_templates;
    uint16_t next_npc_id;
    uint32_t mob_spawn_tick = 0;
    CombatController combat_controller;
    BankService bank_service;
    MerchantService merchant_service;
    SpawnService spawn_service;
    GroundItemService ground_item_service;
    MapTransitionService map_transition_service;
    PlayerSessionService player_session_service;
    CheatService cheat_service;
    uint32_t tick_count = 0;
    int tick_rate_hz;
    bool cheats_enabled;
    std::vector<std::string> help_lines;
    std::unordered_map<uint16_t, double> hp_regen_accum;
    std::unordered_map<uint16_t, double> mana_regen_accum;

    struct PendingResurrection {
        uint32_t remaining_ticks;
        std::string target_map;
        Position target_pos;
    };
    std::unordered_map<uint16_t, PendingResurrection> pending_resurrections_;

    CommandResult apply_regen();
    CommandResult process_pending_resurrections();

    CommandResult handle_move(uint16_t player_id, const MoveCmd& cmd);
    CommandResult handle_attack(uint16_t player_id, const AttackCmd& cmd);
    CommandResult handle_cast_spell(uint16_t player_id, const CastSpellCmd& cmd);
    CommandResult handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd);
    CommandResult handle_resurrect(uint16_t player_id);
    CommandResult handle_meditate(uint16_t player_id);
    CommandResult handle_equip(uint16_t player_id, const EquipItemCmd& cmd);
    CommandResult handle_unequip(uint16_t player_id, const UnequipItemCmd& cmd);
    CommandResult handle_npc_heal(uint16_t player_id);

    std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> compute_combat_ranges(const Player& p) const;
    PlayerStatsEvent make_player_stats_event(const Player& p) const;
    std::optional<uint16_t> find_player_id_by_name(const std::string& name) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id,
                                                  const std::string& map_name) const;
    void append_existing_entities(std::vector<ServerEvent>& events, uint16_t exclude_id,
                                  const std::string& map_name) const;
    Map& player_map(const Player& p);
    const Map& player_map(const Player& p) const;
    bool target_in_safe_zone(uint16_t target_id) const;

public:
    std::string get_player_map_name(uint16_t player_id) const;
    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;
    explicit Game(const ServerConfig& config, PlayerDataService& player_data_service,
                  ClanPersistence& clan_persistence);

    CommandResult process_command(uint16_t player_id, const ClientCommand& cmd);
    CommandResult remove_player(uint16_t player_id);
    void save_all_players();
    CommandResult tick();
};

#endif  // GAME_H
