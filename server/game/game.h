#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../../common/messages.h"
#include "../../common/rng.h"
#include "../core/config.h"
#include "../persistence/clan_persistence.h"
#include "prop_grid.h"

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
    int mob_spawn_radius;
    const ItemCatalog& item_catalog;
    Rng rng;
    CombatController combat_controller;
    uint32_t tick_count = 0;
    int tick_rate_hz;
    bool cheats_enabled;
    std::vector<std::string> help_lines;
    std::unordered_map<uint16_t, double> hp_regen_accum;
    std::unordered_map<uint16_t, double> mana_regen_accum;
    std::map<uint16_t, EnemyNpc> enemy_npcs;
    std::vector<NpcTemplate> npc_templates;
    uint16_t next_npc_id = 2000;
    uint32_t mob_spawn_tick = 0;

    // mapa -> celda (tile_x, tile_y) -> lista de items tirados en el piso
    std::map<std::string, std::map<std::pair<int, int>, std::vector<ItemDroppedEvent>>>
            ground_items;

    struct PendingResurrection {
        uint32_t remaining_ticks;
        std::string target_map;
        Position target_pos;
    };
    std::unordered_map<uint16_t, PendingResurrection> pending_resurrections_;

    double recovery_rate_for(Race race) const;
    double intelligence_for(Race race) const;
    double meditation_factor_for(PlayerClass player_class) const;
    CommandResult apply_regen();
    CommandResult process_pending_resurrections();
    CommandResult spawn_mobs();
    EnemyNpc create_random_npc(Position pos, uint8_t level);

    CommandResult handle_login(uint16_t player_id, const LoginCmd& cmd);
    CommandResult handle_create_character(uint16_t player_id, const CreateCharacterCmd& cmd);
    CommandResult handle_move(uint16_t player_id, const MoveCmd& cmd);
    CommandResult handle_attack(uint16_t player_id, const AttackCmd& cmd);
    CommandResult handle_cast_spell(uint16_t player_id, const CastSpellCmd& cmd);
    CommandResult handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd);
    CommandResult handle_cheat_infinite_hp(uint16_t player_id);
    CommandResult handle_cheat_infinite_mana(uint16_t player_id);
    CommandResult handle_cheat_die(uint16_t player_id);
    CommandResult handle_cheat_level_up(uint16_t player_id);
    CommandResult handle_cheat_level_down(uint16_t player_id);
    CommandResult handle_cheat_add_gold(uint16_t player_id);
    CommandResult handle_cheat_reset_gold(uint16_t player_id);
    CommandResult handle_cheat_velocity(uint16_t player_id);
    CommandResult handle_cheat_revive(uint16_t player_id);
    CommandResult handle_cheat_fill_inventory(uint16_t player_id);
    CommandResult handle_cheat_clear_inventory(uint16_t player_id);
    CommandResult handle_cheat_reset_mana(uint16_t player_id);
    CommandResult handle_help();
    CommandResult handle_change_map(uint16_t player_id, const ChangeMapCmd& cmd);
    CommandResult handle_resurrect(uint16_t player_id);
    CommandResult handle_meditate(uint16_t player_id);
    CommandResult handle_equip(uint16_t player_id, const EquipItemCmd& cmd);
    CommandResult handle_unequip(uint16_t player_id, const UnequipItemCmd& cmd);
    CommandResult handle_npc_heal(uint16_t player_id);
    CommandResult handle_npc_list(uint16_t player_id);
    CommandResult handle_bank_deposit(uint16_t player_id, const BankDepositCmd& cmd);
    CommandResult handle_bank_withdraw(uint16_t player_id, const BankWithdrawCmd& cmd);
    BankUpdateEvent make_bank_update_event(const Player& p) const;
    CommandResult handle_pickup_item(uint16_t player_id, const PickupItemCmd& cmd);
    CommandResult handle_drop_item(uint16_t player_id, const DropItemCmd& cmd);
    static std::pair<int, int> tile_cell(const Map& map, int px, int py);
    static Position cell_center_pos(int tile_size, std::pair<int, int> cell);

    std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> compute_combat_ranges(const Player& p) const;
    PlayerStatsEvent make_player_stats_event(const Player& p) const;
    CommandResult handle_npc_buy(uint16_t player_id, const NpcBuyCmd& cmd);
    CommandResult handle_npc_sell(uint16_t player_id, const NpcSellCmd& cmd);

    struct VendorContext {
        Player* player;
        Map* map;
        int px;
        int py;
        int range;
    };
    std::variant<VendorContext, CommandResult> resolve_vendor_ctx(uint16_t player_id,
                                                                  const std::string& item_name,
                                                                  const std::string& action);
    bool is_username_logged_in(const std::string& username) const;
    LoginOkEvent make_login_ok(const Player& p) const;
    EntitySpawnEvent make_entity_spawn(const Player& p) const;
    EntitySpawnEvent make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id,
                                                  const std::string& map_name) const;
    std::vector<ServerEvent> make_existing_ground_items(const std::string& map_name) const;
    void append_existing_entities(std::vector<ServerEvent>& events, uint16_t exclude_id,
                                  const std::string& map_name) const;
    void commit_ground_drops(CommandResult& result,
                             const std::map<std::string, std::vector<ItemDroppedEvent>>& drops);
    Map& player_map(const Player& p);
    const Map& player_map(const Player& p) const;
    bool try_map_transition(Player& player, CommandResult& result);
    void do_transition(Player& player, CommandResult& result, const PropGrid::Entry& entry,
                       const std::string& old_map_name);

    Position compute_spawn_position(const Map& dest_map, const std::string& old_map_name,
                                    const PropGrid::Entry& source_entry) const;
    void despawn_player(CommandResult& result, uint16_t player_id,
                        const std::string& old_map_name) const;
    void notify_player_transition(CommandResult& result, const Player& player,
                                  const std::string& map_name, Position spawn) const;
    void notify_others_spawn(CommandResult& result, const Player& player,
                             const std::string& map_name) const;

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
