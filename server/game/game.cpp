#include "game.h"


#include <unordered_set>

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

}  // namespace

Game::Game(const ServerConfig& config, PlayerPersistence& persistence):
        persistence(persistence),
        map(config.tilemap),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance),
        combat_controller(config.attack, players) {}

CommandResult Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(overloaded{
                               [&](const LoginCmd& cmd) { return handle_login(player_id, cmd); },
                               [&](const MoveCmd& cmd) { return handle_move(player_id, cmd); },
                               [&](const AttackCmd& cmd) { return handle_attack(player_id, cmd); },
                               [&](const SendChatMsgCmd& cmd) { return handle_send_chat_msg(player_id, cmd); },
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

    EntityDespawnEvent despawn{.entity_id = player_id};
    players.erase(it);
    return {.private_events = {}, .broadcast_events = {despawn}};
}

CommandResult Game::tick() {
    ++tick_count;
    return {};
}

CommandResult Game::handle_attack(uint16_t player_id, const AttackCmd& cmd) {
    return combat_controller.melee_attack(player_id, cmd.target_id, tick_count);
}

CommandResult Game::handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end()) {
        return {};
    }

    const std::string& text = cmd.text;
    const std::string& sender_name = it->second.username;

    if (text.empty()) {
        return {};
    }

    // @nick mensaje — mensaje privado
    if (text[0] == '@') {
        size_t space_pos = text.find(' ');
        if (space_pos != std::string::npos) {
            std::string target_nick = text.substr(1, space_pos - 1);
            std::string msg = text.substr(space_pos + 1);

            for (const auto& [target_id, player]: players) {
                if (player.username == target_nick) {
                    ChatMsgEvent chat_ev{ChatMsgType::PRIVATE, sender_name, msg};
                    return {.private_events = {},
                            .broadcast_events = {},
                            .targeted_events = {{target_id, {chat_ev}}}};
                }
            }

            ChatMsgEvent err{ChatMsgType::SYSTEM, "",
                             "Jugador " + target_nick + " no encontrado"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }
    }

    // /comando — validar contra lista de comandos conocidos
    if (text[0] == '/') {
        std::string cmd_name;
        size_t space_pos = text.find(' ');
        if (space_pos != std::string::npos) {
            cmd_name = text.substr(0, space_pos);
        } else {
            cmd_name = text;
        }

        static const std::unordered_set<std::string> known_commands = {
                "/meditar",    "/resucitar",      "/curar",        "/depositar",
                "/retirar",    "/listar",         "/comprar",      "/vender",
                "/tomar",      "/tirar",          "/fundar-clan",  "/unirse",
                "/revisar-clan", "/clan-aceptar", "/clan-rechazar", "/clan-ban",
                "/dejar-clan", "/clan-kick",
        };

        bool recognized = known_commands.find(cmd_name) != known_commands.end();
        std::string msg = recognized
                                  ? "Comando " + cmd_name + " reconocido"
                                  : "Comando " + cmd_name + " no reconocido";
        ChatMsgEvent response{ChatMsgType::SYSTEM, "", msg};
        return {.private_events = {response}, .broadcast_events = {}, .targeted_events = {}};
    }

    // Texto plano — broadcast a todos
    ChatMsgEvent broadcast_ev{ChatMsgType::SAY, sender_name, text};
    return {.private_events = {}, .broadcast_events = {broadcast_ev}, .targeted_events = {}};
}

CommandResult Game::handle_login(uint16_t player_id, const LoginCmd& cmd) {
    PlayerRecord rec;

    if (persistence.load(cmd.username, rec)) {
        if (!rec.check_password(cmd.password)) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid password"};
            return {.private_events = {err}, .broadcast_events = {}};
        }

        if (is_username_logged_in(cmd.username)) {
            LoginErrorEvent err{LoginError::ALREADY_LOGGED_IN, "This user is already logged in"};
            return {.private_events = {err}, .broadcast_events = {}};
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
        return {.private_events = std::move(private_events), .broadcast_events = {spawn}};
    }

    LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid username or password"};
    return {.private_events = {err}, .broadcast_events = {}};
}

bool Game::is_username_logged_in(const std::string& username) const {
    for (const auto& [id, player]: players) {
        if (player.username == username)
            return true;
    }
    return false;
}

CommandResult Game::handle_move(uint16_t player_id, const MoveCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;

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
    return {.private_events = {}, .broadcast_events = {move}};
}
