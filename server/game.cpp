#include "game.h"

#include <algorithm>

namespace {

int16_t clamp_coord(int16_t value, int16_t min_value, int16_t max_value) {
    return std::max<int16_t>(min_value, std::min<int16_t>(value, max_value));
}
}  // namespace

Game::Game(uint16_t player_id, const ServerConfig& config):
        player{player_id, "hero", {300, 160}, Direction::SOUTH, Race::HUMAN, Class::WARRIOR,
               config.balance},
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        world_w(static_cast<uint16_t>(config.tilemap.mapa.front().size() *
                                      config.tilemap.tile_size)),
        world_h(static_cast<uint16_t>(config.tilemap.mapa.size() *
                                      config.tilemap.tile_size)) {}

std::vector<ServerEvent> Game::get_initial_events() {
    std::vector<ServerEvent> events;

    events.emplace_back(LoginOkEvent{
            .player_id = player.id,
            .username = player.username,
            .race = player.race,
            .class_ = player.class_,
            .level = player.level,
            .experience = player.experience,
            .hp_current = player.hp_current,
            .hp_max = player.hp_max,
            .mana_current = player.mana_current,
            .mana_max = player.mana_max,
            .gold = player.gold,
            .pos = player.pos,
    });

    events.emplace_back(EntitySpawnEvent{
            .entity_id = player.id,
            .entity_type = EntityType::PLAYER,
            .entity_pos = player.pos,
            .entity_dir = player.dir,
            .entity_name = player.username,
            .entity_race = player.race,
            .entity_class = player.class_,
    });

    return events;
}

std::vector<ServerEvent> Game::process_command(const ClientCommand& cmd) {
    if (std::holds_alternative<MoveCmd>(cmd)) {
        return handle_move(std::get<MoveCmd>(cmd));
    }
    return {};
}

std::vector<ServerEvent> Game::tick() { return {}; }

std::vector<ServerEvent> Game::handle_move(const MoveCmd& cmd) {
    int16_t dx = 0;
    int16_t dy = 0;
    switch (cmd.direction) {
        case Direction::NORTH:
            dy = -move_step;
            break;
        case Direction::SOUTH:
            dy = move_step;
            break;
        case Direction::WEST:
            dx = -move_step;
            break;
        case Direction::EAST:
            dx = move_step;
            break;
    }

    const int16_t current_x = static_cast<int16_t>(player.pos.x);
    const int16_t current_y = static_cast<int16_t>(player.pos.y);
    int16_t new_x = static_cast<int16_t>(current_x + dx);
    int16_t new_y = static_cast<int16_t>(current_y + dy);

    const int16_t max_x = static_cast<int16_t>(world_w - sprite_width);
    const int16_t max_y = static_cast<int16_t>(world_h - sprite_height);
    new_x = clamp_coord(new_x, 0, max_x);
    new_y = clamp_coord(new_y, 0, max_y);

    const int final_dx = static_cast<int>(new_x) - static_cast<int>(current_x);
    const int final_dy = static_cast<int>(new_y) - static_cast<int>(current_y);
    if (final_dx == 0 && final_dy == 0) {
        return {};
    }

    player.apply_move(cmd.direction, final_dx, final_dy);

    std::vector<ServerEvent> events;
    events.emplace_back(EntityMoveEvent{
            .entity_id = player.id,
            .entity_pos = player.pos,
            .entity_dir = player.dir,
    });
    return events;
}
