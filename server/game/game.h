#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "../../common/messages.h"
#include "../core/config.h"
#include "../persistence/player_persistence.h"

#include "clan_command_handler.h"
#include "clan_manager.h"
#include "combat_controller.h"
#include "command_result.h"
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
    CombatController combat_controller;
    uint32_t tick_count = 0;

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
    CommandResult handle_resurrect(uint16_t player_id);
    CommandResult handle_meditate(uint16_t player_id);
    bool is_username_logged_in(const std::string& username) const;
    LoginOkEvent make_login_ok(const Player& p) const;
    EntitySpawnEvent make_entity_spawn(const Player& p) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id) const;
    std::vector<ServerEvent> make_existing_spawns(uint16_t exclude_id, const std::string& map_name) const;
    Map& player_map(const Player& p);
    const Map& player_map(const Player& p) const;
    std::string get_player_map_name(uint16_t player_id) const;
    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;
    bool try_map_transition(Player& player, CommandResult& result);

public:
    explicit Game(const ServerConfig& config, PlayerPersistence& persistence,
                  ClanPersistence& clan_persistence);

    CommandResult process_command(uint16_t player_id, const ClientCommand& cmd);
    CommandResult remove_player(uint16_t player_id);
    void save_all_players();
    CommandResult tick();
};

#endif  // GAME_H
