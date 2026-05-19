#include "game.h"

Game::Game(uint16_t player_id, const ServerConfig& config):
        player{player_id, "hero", {300, 160}, Direction::SOUTH, Race::HUMAN, Class::WARRIOR,
               config.balance},
        map(config.tilemap),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height) {}

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

    const int current_x = static_cast<int>(player.pos.x);
    const int current_y = static_cast<int>(player.pos.y);
    const int new_x = map.clamp_x(current_x + dx, sprite_width);
    const int new_y = map.clamp_y(current_y + dy, sprite_height);

    if (!map.is_walkable(new_x + sprite_width / 2, new_y + sprite_height)) {
        return {};
    }

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
