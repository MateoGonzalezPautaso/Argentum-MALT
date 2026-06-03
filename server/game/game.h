#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../common/equipable_items.h"
#include "../../common/messages.h"
#include "../../common/rng.h"
#include "../core/config.h"
#include "../persistence/player_persistence.h"

#include "clan_command_handler.h"
#include "clan_manager.h"
#include "combat_controller.h"
#include "command_result.h"
#include "enemy_npc.h"
#include "map.h"
#include "player.h"

class Game {
private:
    std::map<uint16_t, Player> players;
    PlayerPersistence& persistence;
    ClanManager clan_manager;
    ClanCommandHandler clan_handler;
    std::unordered_map<std::string, TilemapConfig> tilemap_configs;
    std::unordered_map<std::string, Map> maps;
    int move_step;
    int sprite_width;
    int sprite_height;
    BalanceConfig balance;
    Rng rng;
    CombatController combat_controller;
    uint32_t tick_count = 0;
    int tick_rate_hz;
    std::unordered_map<uint16_t, double> hp_regen_accum;
    std::unordered_map<uint16_t, double> mana_regen_accum;
    std::map<uint16_t, EnemyNpc> npcs;
    EquipableItems equipable_items;


    double recovery_rate_for(Race race) const;
    double intelligence_for(Race race) const;
    double meditation_factor_for(PlayerClass player_class) const;
    CommandResult apply_regen();

    CommandResult handle_login(uint16_t player_id, const LoginCmd& cmd);
    CommandResult handle_create_character(uint16_t player_id, const CreateCharacterCmd& cmd);
    CommandResult handle_move(uint16_t player_id, const MoveCmd& cmd);
    CommandResult handle_attack(uint16_t player_id, const AttackCmd& cmd);
    CommandResult handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd);
    CommandResult handle_cheat_infinite_hp(uint16_t player_id);
    CommandResult handle_cheat_infinite_mana(uint16_t player_id);
    CommandResult handle_cheat_die(uint16_t player_id);
    CommandResult handle_cheat_level_up(uint16_t player_id);
    CommandResult handle_cheat_level_down(uint16_t player_id);
    CommandResult handle_cheat_add_gold(uint16_t player_id);
    CommandResult handle_cheat_velocity(uint16_t player_id);
    CommandResult handle_change_map(uint16_t player_id, const ChangeMapCmd& cmd);
    CommandResult handle_resurrect(uint16_t player_id);
    CommandResult handle_meditate(uint16_t player_id);
    bool is_username_logged_in(const std::string& username) const;
    LoginOkEvent make_login_ok(const Player& p) const;
    EntitySpawnEvent make_entity_spawn(const Player& p) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id,
                                                  const std::string& map_name) const;
    Map& player_map(const Player& p);
    const Map& player_map(const Player& p) const;
    bool try_map_transition(Player& player, CommandResult& result);
    void do_transition(Player& player, CommandResult& result, const PropDef& prop,
                       const std::string& old_map_name);

    Position compute_spawn_position(const TilemapConfig& dest_cfg, const PropDef& prop) const;
    void despawn_player(CommandResult& result, uint16_t player_id,
                        const std::string& old_map_name) const;
    void notify_player_transition(CommandResult& result, const Player& player,
                                  const std::string& map_name, Position spawn) const;
    void notify_others_spawn(CommandResult& result, const Player& player,
                             const std::string& map_name) const;

public:
    std::string get_player_map_name(uint16_t player_id) const;
    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;
    explicit Game(const ServerConfig& config, PlayerPersistence& persistence,
                  ClanPersistence& clan_persistence);

    CommandResult process_command(uint16_t player_id, const ClientCommand& cmd);
    CommandResult remove_player(uint16_t player_id);
    void save_all_players();
    CommandResult tick();
};

#endif  // GAME_H
