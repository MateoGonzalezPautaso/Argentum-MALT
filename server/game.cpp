#include "game.h"

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

Game::Game(const ServerConfig& config):
        map(config.tilemap),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance) {}

std::vector<ServerEvent> Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(overloaded{
                              [&](const LoginCmd& cmd) { return handle_login(player_id, cmd); },
                              [&](const MoveCmd& cmd) { return handle_move(player_id, cmd); },
                              [](const auto&) { return std::vector<ServerEvent>{}; },
                      },
                      cmd);
}

std::vector<ServerEvent> Game::tick() { return {}; }

std::vector<ServerEvent> Game::handle_login(uint16_t player_id, const LoginCmd& cmd) {
    Player new_player(player_id, cmd.username, Position{300, 160},
                      Direction::SOUTH, Race::HUMAN, PlayerClass::WARRIOR, balance);
    auto it = players.emplace(player_id, std::move(new_player)).first;
    const Player& p = it->second;

    LoginOkEvent login_ok{
            .player_id = p.id,
            .username = p.username,
            .race = p.race,
            .player_class = p.player_class,
            .level = p.level,
            .experience = p.experience,
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
    return {login_ok, spawn};
}

std::vector<ServerEvent> Game::handle_move(uint16_t player_id, const MoveCmd& cmd) {
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

    std::vector<ServerEvent> events;
    events.emplace_back(EntityMoveEvent{
            .entity_id = player.id,
            .entity_pos = player.pos,
            .entity_dir = player.dir,
    });
    return events;
}
