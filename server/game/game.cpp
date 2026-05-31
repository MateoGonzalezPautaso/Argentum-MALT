#include "game.h"

#include <cstdlib>
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
        clan_manager(clan_persistence, config.clan),
        clan_handler(clan_manager, players),
        tilemap_configs(config.tilemap_configs),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance),
        combat_controller(config.attack, players) {
    for (const auto& [name, tc] : tilemap_configs) {
        maps.emplace(name, Map(tc));
    }
    combat_controller.set_clan_manager(clan_manager);
}

Map& Game::player_map(const Player& p) {
    auto it = maps.find(p.get_current_map());
    if (it == maps.end())
        return maps.begin()->second;
    return it->second;
}

const Map& Game::player_map(const Player& p) const {
    auto it = maps.find(p.get_current_map());
    if (it == maps.end())
        return maps.begin()->second;
    return it->second;
}

CommandResult Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(overloaded{
                               [&](const LoginCmd& cmd) { return handle_login(player_id, cmd); },
                               [&](const CreateCharacterCmd& cmd) { return handle_create_character(player_id, cmd); },
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
                                [&](const ChangeMapCmd& cmd) { return handle_change_map(player_id, cmd); },
                                [](const auto&) { return CommandResult{}; },
                       },
                       cmd);
}

CommandResult Game::remove_player(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    std::string map_name = it->second.get_current_map();
    std::string clan_name = it->second.get_clan_name();
    std::string username = it->second.get_username();
    persistence.save(it->second);

    EntityDespawnEvent despawn{.entity_id = player_id};

    // Send despawn to all players on the same map
    CommandResult result;
    for (uint16_t pid : get_player_ids_on_map(map_name)) {
        if (pid == player_id) continue;
        result.targeted_events[pid].push_back(despawn);
    }

    players.erase(it);

    // Notify clan members of logout
    if (!clan_name.empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_OFFLINE, username, clan_name};
        auto clan_result = clan_handler.notify_clan_members(notif.clan_name, notif, player_id);
        for (auto& ev: clan_result.targeted_events)
            for (auto& se: ev.second)
                result.targeted_events[ev.first].push_back(std::move(se));
    }

    return result;
}

CommandResult Game::tick() {
    ++tick_count;
    return {};
}

CommandResult Game::handle_attack(uint16_t player_id, const AttackCmd& cmd) {
    auto it = players.find(player_id);
    if (it != players.end())
        it->second.set_meditating(false);
    CommandResult result = combat_controller.melee_attack(player_id, cmd.target_id, tick_count);
    // Convert combat broadcasts to per-map events
    result.map_events = std::move(result.broadcast_events);
    return result;
}

CommandResult Game::handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& text = cmd.text;
    if (text.empty())
        return {};

    it->second.set_meditating(false);

    const std::string& sender_name = it->second.get_username();

    if (text[0] == '@') {
        size_t space_pos = text.find(' ');
        if (space_pos != std::string::npos) {
            std::string target_nick = text.substr(1, space_pos - 1);
            std::string msg = text.substr(space_pos + 1);
            for (const auto& [target_id, player]: players) {
                if (player.get_username() == target_nick) {
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
                       static_cast<PlayerClass>(rec.player_class), balance,
                       rec.level, rec.experience,
                       rec.hp_current, rec.hp_max,
                       rec.mana_current, rec.mana_max,
                       rec.gold);

        player.set_current_map(rec.get_current_map());
        if (maps.find(player.get_current_map()) == maps.end())
            player.set_current_map("main");

        // Set clan membership
        std::string clan_name = clan_manager.get_clan_name(cmd.username);
        player.set_clan_name(clan_name);

        auto it = players.emplace(player_id, std::move(player)).first;
        const Player& p = it->second;

        EntitySpawnEvent spawn = make_entity_spawn(p);
        std::vector<ServerEvent> private_events = {make_login_ok(p)};

        if (p.get_current_map() != "main") {
            private_events.push_back(MapTransitionEvent{
                .map_name = p.get_current_map(),
                .pos_x = p.pos_x(),
                .pos_y = p.pos_y(),
            });
        }

        auto existing = make_existing_spawns(p.get_id(), p.get_current_map());
        private_events.insert(private_events.end(), existing.begin(), existing.end());

        // Notify clan members of login
        CommandResult login_result;
        login_result.private_events = std::move(private_events);
        login_result.map_events = {spawn};

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

CommandResult Game::handle_create_character(uint16_t player_id, const CreateCharacterCmd& cmd) {
    if (cmd.username.empty()) {
        CharacterErrorEvent err{CharacterError::INVALID_USERNAME, "El nombre de usuario no puede estar vacio"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    PlayerRecord existing;
    if (persistence.load(cmd.username, existing)) {
        CharacterErrorEvent err{CharacterError::USERNAME_TAKEN, "El usuario " + cmd.username + " ya existe"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (is_username_logged_in(cmd.username)) {
        CharacterErrorEvent err{CharacterError::USERNAME_TAKEN, "El usuario " + cmd.username + " ya esta en juego"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    PlayerRecord rec;
    rec.set_username(cmd.username);
    rec.set_password(cmd.password);
    rec.race = static_cast<uint8_t>(cmd.race);
    rec.player_class = static_cast<uint8_t>(cmd.player_class);
    rec.level = 1;
    rec.experience = 0;
    rec.pos_x = static_cast<uint16_t>(balance.starting_pos_x);
    rec.pos_y = static_cast<uint16_t>(balance.starting_pos_y);
    rec.dir = static_cast<uint8_t>(Direction::SOUTH);
    rec.hp_current = static_cast<uint32_t>(balance.starting_hp);
    rec.hp_max = static_cast<uint32_t>(balance.starting_hp);
    rec.mana_current = static_cast<uint32_t>(balance.starting_mana);
    rec.mana_max = static_cast<uint32_t>(balance.starting_mana);
    rec.gold = static_cast<uint32_t>(balance.starting_gold);
    persistence.save(cmd.username, rec);

    Player player(player_id, cmd.username,
                  Position{rec.pos_x, rec.pos_y},
                  Direction::SOUTH, cmd.race, cmd.player_class, balance);
    auto it = players.emplace(player_id, std::move(player)).first;
    const Player& p = it->second;

    CharacterCreatedEvent created{make_login_ok(p)};
    EntitySpawnEvent spawn = make_entity_spawn(p);
    std::vector<ServerEvent> private_events = {created};
    auto other_spawns = make_existing_spawns(p.get_id(), p.get_current_map());
    private_events.insert(private_events.end(), other_spawns.begin(), other_spawns.end());

    CommandResult r;
    r.private_events = std::move(private_events);
    r.map_events = {spawn};
    return r;
}

CommandResult Game::handle_cheat_infinite_hp(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    bool active = it->second.toggle_cheat_infinite_hp();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? "[Cheat] Infinite HP: ON" : "[Cheat] Infinite HP: OFF"};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_infinite_mana(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    bool active = it->second.toggle_cheat_infinite_mana();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? "[Cheat] Infinite Mana: ON" : "[Cheat] Infinite Mana: OFF"};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_die(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    it->second.kill();
    DamageReceivedEvent dmg{.target_id = player_id, .attacker_id = player_id,
                            .damage = 0, .hp_current = 0, .hp_max = it->second.get_hp_max()};
    EntityDiedEvent died{.entity_id = player_id};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] You died!"};
    CommandResult r;
    r.private_events = {msg};
    r.map_events = {dmg, died};
    return r;
}

CommandResult Game::handle_cheat_level_up(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    player.level_up();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel subido a " + std::to_string(player.get_level())};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_level_down(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    if (player.get_level() <= 1) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Ya estas en el nivel minimo"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    player.level_down();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel bajado a " + std::to_string(player.get_level())};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_meditate(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;

    if (player.is_ghost())
        return {};

    if (player.get_player_class() == PlayerClass::WARRIOR) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los guerreros no pueden meditar"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (player.get_is_meditating()) {
        player.set_meditating(false);
        return {.private_events = {MeditationStopEvent{}}, .broadcast_events = {}, .targeted_events = {}};
    } else {
        player.set_meditating(true);
        return {.private_events = {MeditationStartEvent{}}, .broadcast_events = {}, .targeted_events = {}};
    }
}

LoginOkEvent Game::make_login_ok(const Player& p) const {
    return LoginOkEvent{
            .player_id = p.get_id(),
            .username = p.get_username(),
            .race = p.get_race(),
            .player_class = p.get_player_class(),
            .level = p.get_level(),
            .experience = p.get_experience(),
            .exp_to_next = p.exp_to_next_level(),
            .hp_current = p.get_hp_current(),
            .hp_max = p.get_hp_max(),
            .mana_current = p.get_mana_current(),
            .mana_max = p.get_mana_max(),
            .gold = p.get_gold(),
            .pos = p.get_pos(),
    };
}

EntitySpawnEvent Game::make_entity_spawn(const Player& p) const {
    return EntitySpawnEvent{
            .entity_id = p.get_id(),
            .entity_type = EntityType::PLAYER,
            .entity_pos = p.get_pos(),
            .entity_dir = p.get_dir(),
            .entity_name = p.get_username(),
            .entity_race = p.get_race(),
            .entity_class = p.get_player_class(),
    };
}

std::vector<ServerEvent> Game::make_existing_spawns(uint16_t exclude_id) const {
    std::vector<ServerEvent> spawns;
    for (const auto& [id, player]: players) {
        if (id == exclude_id)
            continue;
        spawns.push_back(make_entity_spawn(player));
    }
    return spawns;
}

std::vector<ServerEvent> Game::make_existing_spawns(uint16_t exclude_id, const std::string& map_name) const {
    std::vector<ServerEvent> spawns;
    for (const auto& [id, player]: players) {
        if (id == exclude_id)
            continue;
        if (player.get_current_map() != map_name)
            continue;
        spawns.push_back(make_entity_spawn(player));
    }
    return spawns;
}

std::string Game::get_player_map_name(uint16_t player_id) const {
    auto it = players.find(player_id);
    if (it == players.end())
        return "main";
    return it->second.get_current_map();
}

std::vector<uint16_t> Game::get_player_ids_on_map(const std::string& map_name) const {
    std::vector<uint16_t> ids;
    for (const auto& [id, player]: players) {
        if (player.get_current_map() == map_name)
            ids.push_back(id);
    }
    return ids;
}

void Game::save_all_players() {
    for (const auto& [id, player]: players) {
        persistence.save(player);
    }
}

bool Game::is_username_logged_in(const std::string& username) const {
    for (const auto& [id, player]: players) {
        if (player.get_username() == username)
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

    PlayerRespawnedEvent respawn_ev{player_id, player.get_hp_current(), player.get_hp_max()};

    CommandResult r;
    r.map_events = {respawn_ev};
    return r;
}

CommandResult Game::handle_move(uint16_t player_id, const MoveCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    player.set_meditating(false);

    Map& cur_map = player_map(player);

    auto [dx, dy] = direction_to_delta(cmd.direction, move_step);

    const int current_x = static_cast<int>(player.pos_x());
    const int current_y = static_cast<int>(player.pos_y());
    const int new_x = cur_map.clamp_x(current_x + dx, sprite_width);
    const int new_y = cur_map.clamp_y(current_y + dy, sprite_height);

    if (!cur_map.is_walkable(new_x + sprite_width / 2, new_y + sprite_height))
        return {};

    for (const auto& [other_id, other]: players) {
        if (other_id == player_id)
            continue;
        if (other.get_current_map() != player.get_current_map())
            continue;
        const int ox = static_cast<int>(other.pos_x());
        const int oy = static_cast<int>(other.pos_y());
        const int hw = sprite_width / 2;
        const int hh = sprite_height / 2;
        bool already_overlapping = (std::abs(current_x - ox) < hw &&
                                    std::abs(current_y - oy) < hh);
        if (already_overlapping)
            continue;
        if (std::abs(new_x - ox) < hw && std::abs(new_y - oy) < hh)
            return {};
    }

    const int final_dx = new_x - current_x;
    const int final_dy = new_y - current_y;
    if (final_dx == 0 && final_dy == 0)
        return {};

    player.apply_move(cmd.direction, final_dx, final_dy);

    CommandResult result;

    if (try_map_transition(player, result))
        return result;

    EntityMoveEvent move{
            .entity_id = player.get_id(),
            .entity_pos = player.get_pos(),
            .entity_dir = player.get_dir(),
    };
    result.map_events = {move};
    return result;
}

bool Game::try_map_transition(Player& player, CommandResult& result) {
    const std::string old_map_name = player.get_current_map();
    auto map_it = maps.find(old_map_name);
    if (map_it == maps.end())
        return false;
    const TilemapConfig& cfg = map_it->second.config();
    const int foot_x = static_cast<int>(player.pos_x()) + sprite_width / 2;
    const int foot_y = static_cast<int>(player.pos_y()) + sprite_height;

    for (std::size_t r = 0; r < cfg.prop_map.size(); ++r) {
        for (std::size_t c = 0; c < cfg.prop_map[r].size(); ++c) {
            const auto& prop_name = cfg.prop_map[r][c];
            if (prop_name.empty()) continue;

            auto prop_it = cfg.props.find(prop_name);
            if (prop_it == cfg.props.end()) continue;

            const auto& prop = prop_it->second;
            if (prop.transition_map.empty()) continue;

            int prop_x = static_cast<int>(c) * cfg.tile_size;
            int prop_y = (static_cast<int>(r) + 1) * cfg.tile_size - prop.height;

            int hb_left = prop_x + prop.hitbox.x;
            int hb_top = prop_y + prop.hitbox.y;
            int hb_right = hb_left + prop.hitbox.w;
            int hb_bottom = hb_top + prop.hitbox.h;

            if (foot_x >= hb_left && foot_x < hb_right &&
                foot_y >= hb_top && foot_y < hb_bottom) {

                do_transition(player, result, prop, old_map_name);
                return true;
            }
        }
    }
    return false;
}

void Game::do_transition(Player& player, CommandResult& result, const PropDef& prop,
                         const std::string& old_map_name) {
    auto dest_it = maps.find(prop.transition_map);
    if (dest_it == maps.end()) return;

    // Despawn on old map
    EntityDespawnEvent despawn{.entity_id = player.get_id()};
    for (uint16_t pid : get_player_ids_on_map(old_map_name)) {
        if (pid == player.get_id()) continue;
        result.targeted_events[pid].push_back(despawn);
    }

    const TilemapConfig& cfg = dest_it->second.config();
    int spawn_x = prop.transition_x;
    int spawn_y = prop.transition_y;
    if (spawn_x == 0 && spawn_y == 0) {
        spawn_x = cfg.tile_size * 2;
        spawn_y = cfg.tile_size * 2;
    }

    player.set_current_map(prop.transition_map);
    player.set_pos(static_cast<uint16_t>(spawn_x),
                   static_cast<uint16_t>(spawn_y));

    persistence.save(player);

    result.private_events.push_back(MapTransitionEvent{
        .map_name = prop.transition_map,
        .pos_x = static_cast<uint16_t>(spawn_x),
        .pos_y = static_cast<uint16_t>(spawn_y),
    });

    // Send existing spawns on new map to the player
    std::vector<ServerEvent> others = make_existing_spawns(player.get_id());
    result.private_events.insert(result.private_events.end(), others.begin(), others.end());

    // Spawn player on new map for other players there
    EntitySpawnEvent spawn = make_entity_spawn(player);
    for (uint16_t pid : get_player_ids_on_map(prop.transition_map)) {
        if (pid == player.get_id()) continue;
        result.targeted_events[pid].push_back(spawn);
    }
}

CommandResult Game::handle_change_map(uint16_t player_id, const ChangeMapCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    const std::string& old_map_name = player.get_current_map();
    auto map_it = maps.find(old_map_name);
    if (map_it == maps.end())
        return {};

    const TilemapConfig& cfg = map_it->second.config();

    // Find the prop definition
    auto prop_it = cfg.props.find(cmd.prop_name);
    if (prop_it == cfg.props.end())
        return {};

    const PropDef& prop = prop_it->second;
    if (prop.transition_map.empty())
        return {};

    // Validate player is within interaction range (3 tiles) of any placement of this prop
    const int range = cfg.tile_size * 3;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());
    bool in_range = false;

    for (std::size_t r = 0; r < cfg.prop_map.size() && !in_range; ++r) {
        for (std::size_t c = 0; c < cfg.prop_map[r].size() && !in_range; ++c) {
            if (cfg.prop_map[r][c] != cmd.prop_name) continue;

            int prop_x = static_cast<int>(c) * cfg.tile_size;
            int prop_y = (static_cast<int>(r) + 1) * cfg.tile_size - prop.height;
            int center_x = prop_x + prop.width / 2;
            int center_y = prop_y + prop.height / 2;

            if (std::abs(px - center_x) < range && std::abs(py - center_y) < range)
                in_range = true;
        }
    }

    if (!in_range)
        return {};

    CommandResult result;
    do_transition(player, result, prop, old_map_name);
    return result;
}
