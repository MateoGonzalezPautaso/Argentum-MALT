#include "game.h"

#include <utility>
#include <variant>

#include "../common/visit.h"

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
        balance(config.balance) {}

CommandResult Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(overloaded{
                              [&](const LoginCmd& cmd) { return handle_login(player_id, cmd); },
                              [&](const MoveCmd& cmd) { return handle_move(player_id, cmd); },
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

CommandResult Game::tick() { return {}; }

CommandResult Game::handle_login(uint16_t player_id, const LoginCmd& cmd) {
    PlayerRecord rec;

    if (persistence.load(cmd.username, rec)) {
        if (!rec.check_password(cmd.password)) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid password"};
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
        return {.private_events = {login_ok}, .broadcast_events = {spawn}};
    }

    LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid username or password"};
    return {.private_events = {err}, .broadcast_events = {}};
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
