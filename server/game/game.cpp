#include "game.h"

#include <string>

#include <utility>
#include <variant>
#include <vector>

#include "../../common/visit.h"

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

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(' ');
    if (start == std::string::npos)
        return {};
    size_t end = s.find_last_not_of(' ');
    return s.substr(start, end - start + 1);
}

}  // namespace

Game::Game(const ServerConfig& config, PlayerPersistence& persistence,
           ClanPersistence& clan_persistence):
        persistence(persistence),
        clan_manager(clan_persistence),
        clan_handler(clan_manager, players),
        map(config.tilemap),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance),
        combat_controller(config.attack, players) {
    combat_controller.set_clan_manager(clan_manager);
}

CommandResult Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(overloaded{
                               [&](const LoginCmd& cmd) { return handle_login(player_id, cmd); },
                               [&](const MoveCmd& cmd) { return handle_move(player_id, cmd); },
                               [&](const AttackCmd& cmd) { return handle_attack(player_id, cmd); },
                               [&](const SendChatMsgCmd& cmd) { return handle_send_chat_msg(player_id, cmd); },
                               [&](const MeditateCmd&) { return handle_meditate(player_id); },
                               [&](const ResurrectCmd&) { return handle_resurrect(player_id); },
                               [&](const CheatInfiniteHpCmd&) { return handle_cheat_infinite_hp(player_id); },
                               [&](const CheatInfiniteManaCmd&) { return handle_cheat_infinite_mana(player_id); },
                               [&](const CheatDieCmd&) { return handle_cheat_die(player_id); },
                               [&](const CheatLevelUpCmd&) { return handle_cheat_level_up(player_id); },
                               [&](const CheatLevelDownCmd&) { return handle_cheat_level_down(player_id); },
                               [](const auto&) { return CommandResult{}; },
                       },
                       cmd);
}

CommandResult Game::remove_player(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    PlayerRecord rec;
    rec.set_username(it->second.username);
    rec.pos_x = it->second.pos.x;
    rec.pos_y = it->second.pos.y;
    rec.dir = static_cast<uint8_t>(it->second.dir);
    rec.race = static_cast<uint8_t>(it->second.race);
    rec.player_class = static_cast<uint8_t>(it->second.player_class);
    rec.level = it->second.level;
    rec.experience = it->second.experience;
    rec.hp_current = it->second.hp_current;
    rec.hp_max = it->second.hp_max;
    rec.mana_current = it->second.mana_current;
    rec.mana_max = it->second.mana_max;
    rec.gold = it->second.gold;
    persistence.save(it->second.username, rec);

    // Notify clan members of logout
    if (!it->second.clan_name.empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_OFFLINE, it->second.username,
                                    it->second.clan_name};
        EntityDespawnEvent despawn{.entity_id = player_id};
        players.erase(it);
        CommandResult result;
        result.broadcast_events.push_back(despawn);
        auto clan_result = clan_handler.notify_clan_members(notif.clan_name, notif, player_id);
        for (auto& ev: clan_result.targeted_events)
            for (auto& se: ev.second)
                result.targeted_events[ev.first].push_back(std::move(se));
        return result;
    }

    EntityDespawnEvent despawn{.entity_id = player_id};
    players.erase(it);
    return {.private_events = {}, .broadcast_events = {despawn}, .targeted_events = {}};
}

CommandResult Game::tick() {
    ++tick_count;
    return {};
}

CommandResult Game::handle_attack(uint16_t player_id, const AttackCmd& cmd) {
    auto it = players.find(player_id);
    if (it != players.end())
        it->second.is_meditating = false;
    return combat_controller.melee_attack(player_id, cmd.target_id, tick_count);
}

CommandResult Game::handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& text = cmd.text;
    if (text.empty())
        return {};

    it->second.is_meditating = false;

    const std::string& sender_name = it->second.username;

    if (text[0] == '@') {
        size_t space_pos = text.find(' ');
        if (space_pos != std::string::npos) {
            std::string target_nick = text.substr(1, space_pos - 1);
            std::string msg = text.substr(space_pos + 1);
            for (const auto& [target_id, player]: players) {
                if (player.username == target_nick) {
                    ChatMsgEvent chat_ev{ChatMsgType::PRIVATE, sender_name, msg,
                                        target_id, player_id};
                    std::map<uint16_t, std::vector<ServerEvent>> targeted;
                    targeted[target_id].push_back(chat_ev);
                    targeted[player_id].push_back(chat_ev);
                    return {.private_events = {}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
                }
            }
            ChatMsgEvent err{ChatMsgType::SYSTEM, "", "Jugador " + target_nick + " no encontrado"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }
    }

    if (text[0] == '/') {
        size_t space_pos = text.find(' ');
        std::string cmd_name = text.substr(0, space_pos);
        std::string args = (space_pos != std::string::npos) ? trim(text.substr(space_pos + 1)) : "";

        if (auto result = clan_handler.handle(player_id, cmd_name, args))
            return *result;

        if (cmd_name == "/meditar") return handle_meditate(player_id);
        if (cmd_name == "/resucitar") return handle_resurrect(player_id);

        ChatMsgEvent ev{ChatMsgType::SYSTEM, "", "Comando " + cmd_name + " no reconocido"};
        return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = {}};
    }

    ChatMsgEvent broadcast_ev{ChatMsgType::SAY, sender_name, text};
    return {.private_events = {}, .broadcast_events = {broadcast_ev}, .targeted_events = {}};
}


CommandResult Game::handle_login(uint16_t player_id, const LoginCmd& cmd) {
    PlayerRecord rec;

    if (persistence.load(cmd.username, rec)) {
        if (!rec.check_password(cmd.password)) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid password"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        if (is_username_logged_in(cmd.username)) {
            LoginErrorEvent err{LoginError::ALREADY_LOGGED_IN, "This user is already logged in"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        Player player(player_id, cmd.username, Position{rec.pos_x, rec.pos_y},
                      static_cast<Direction>(rec.dir), static_cast<Race>(rec.race),
                      static_cast<PlayerClass>(rec.player_class), balance);
        player.level = rec.level;
        player.experience = rec.experience;
        player.hp_current = rec.hp_current;
        player.hp_max = rec.hp_max;
        player.mana_current = rec.mana_current;
        player.mana_max = rec.mana_max;
        player.gold = rec.gold;

        // Set clan membership
        std::string clan_name = clan_manager.get_clan_name(cmd.username);
        player.clan_name = clan_name;

        auto it = players.emplace(player_id, std::move(player)).first;
        const Player& p = it->second;

        LoginOkEvent login_ok{
                .player_id = p.id,
                .username = p.username,
                .race = p.race,
                .player_class = p.player_class,
                .level = p.level,
                .experience = p.experience,
                .exp_to_next = p.exp_to_next_level(),
                .hp_current = p.hp_current,
                .hp_max = p.hp_max,
                .mana_current = p.mana_current,
                .mana_max = p.mana_max,
                .gold = p.gold,
                .pos = p.pos,
        };
        EntitySpawnEvent spawn{
                .entity_id = p.id,
                .entity_type = EntityType::PLAYER,
                .entity_pos = p.pos,
                .entity_dir = p.dir,
                .entity_name = p.username,
                .entity_race = p.race,
                .entity_class = p.player_class,
        };
        std::vector<ServerEvent> private_events = {login_ok};
        for (const auto& [existing_id, existing_player]: players) {
            if (existing_id == p.id) {
                continue;
            }
            private_events.push_back(EntitySpawnEvent{
                    .entity_id = existing_player.id,
                    .entity_type = EntityType::PLAYER,
                    .entity_pos = existing_player.pos,
                    .entity_dir = existing_player.dir,
                    .entity_name = existing_player.username,
                    .entity_race = existing_player.race,
                    .entity_class = existing_player.player_class,
            });
        }

        // Notify clan members of login
        CommandResult login_result;
        login_result.private_events = std::move(private_events);
        login_result.broadcast_events = {spawn};

        if (!clan_name.empty()) {
            ClanNotificationEvent notif{ClanNotifType::MEMBER_ONLINE, cmd.username, clan_name};
            auto clan_result = clan_handler.notify_clan_members(clan_name, notif, player_id);
            login_result.targeted_events = std::move(clan_result.targeted_events);
        }

        return login_result;
    }

    LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid username or password"};
    return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_infinite_hp(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    it->second.cheat_infinite_hp = !it->second.cheat_infinite_hp;
    bool active = it->second.cheat_infinite_hp;
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? "[Cheat] Infinite HP: ON" : "[Cheat] Infinite HP: OFF"};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_infinite_mana(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    it->second.cheat_infinite_mana = !it->second.cheat_infinite_mana;
    bool active = it->second.cheat_infinite_mana;
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? "[Cheat] Infinite Mana: ON" : "[Cheat] Infinite Mana: OFF"};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_die(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    it->second.hp_current = 0;
    it->second.is_dead = true;
    DamageReceivedEvent dmg{.target_id = player_id, .attacker_id = player_id,
                            .damage = 0, .hp_current = 0, .hp_max = it->second.hp_max};
    EntityDiedEvent died{.entity_id = player_id};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] You died!"};
    return {.private_events = {msg}, .broadcast_events = {dmg, died}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_level_up(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    player.level_up();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel subido a " + std::to_string(player.level)};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_level_down(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    if (player.level <= 1) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Ya estas en el nivel minimo"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    player.level--;
    player.hp_max = std::max(player.hp_max, static_cast<uint32_t>(player.balance.starting_hp));
    player.mana_max = std::max(player.mana_max, static_cast<uint32_t>(player.balance.starting_mana));
    player.hp_current = player.hp_max;
    player.mana_current = player.mana_max;
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel bajado a " + std::to_string(player.level)};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_meditate(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;

    if (player.is_ghost())
        return {};

    if (player.player_class == PlayerClass::WARRIOR) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los guerreros no pueden meditar"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (player.is_meditating) {
        player.is_meditating = false;
        return {.private_events = {MeditationStopEvent{}}, .broadcast_events = {}, .targeted_events = {}};
    } else {
        player.is_meditating = true;
        return {.private_events = {MeditationStartEvent{}}, .broadcast_events = {}, .targeted_events = {}};
    }
}

bool Game::is_username_logged_in(const std::string& username) const {
    for (const auto& [id, player]: players) {
        if (player.username == username)
            return true;
    }
    return false;
}

CommandResult Game::handle_resurrect(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end()) {
        return {};
    }

    Player& player = it->second;
    if (!player.is_ghost()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "You are not dead"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    player.resurrect();

    PlayerRespawnedEvent respawn_ev{player_id, player.hp_current, player.hp_max};

    return {.private_events = {}, .broadcast_events = {respawn_ev}, .targeted_events = {}};
}

CommandResult Game::handle_move(uint16_t player_id, const MoveCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    player.is_meditating = false;

    auto [dx, dy] = direction_to_delta(cmd.direction, move_step);

    const int current_x = static_cast<int>(player.pos.x);
    const int current_y = static_cast<int>(player.pos.y);
    const int new_x = map.clamp_x(current_x + dx, sprite_width);
    const int new_y = map.clamp_y(current_y + dy, sprite_height);

    if (!map.is_walkable(new_x + sprite_width / 2, new_y + sprite_height))
        return {};

    const int final_dx = new_x - current_x;
    const int final_dy = new_y - current_y;
    if (final_dx == 0 && final_dy == 0)
        return {};

    player.apply_move(cmd.direction, final_dx, final_dy);

    EntityMoveEvent move{
            .entity_id = player.id,
            .entity_pos = player.pos,
            .entity_dir = player.dir,
    };
    return {.private_events = {}, .broadcast_events = {move}, .targeted_events = {}};
}
