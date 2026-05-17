#include "game.h"

#include <algorithm>

Game::Game(uint16_t player_id):
        player{
                .id = player_id,
                .username = "hero",
                .pos = {400, 200},
                .dir = Direction::SOUTH,
                .race = Race::HUMAN,
                .class_type = Class::WARRIOR,
                .level = 1,
                .experience = 0,
                .hp_current = 100,
                .hp_max = 100,
                .mana_current = 50,
                .mana_max = 50,
                .gold = 0,
        },
        world_w(1280),
        world_h(1280) {}

std::vector<ServerEvent> Game::get_initial_events() {
    std::vector<ServerEvent> events;

    events.emplace_back(LoginOkEvent{
            .player_id = player.id,
            .username = player.username,
            .race = player.race,
            .class_ = player.class_type,
            .level = player.level,
            .experience = player.experience,
            .hp_current = player.hp_current,
            .hp_max = player.hp_max,
            .mana_current = player.mana_current,
            .mana_max = player.mana_max,
            .gold = player.gold,
            .pos = player.pos,
    });  // Doing this we avoid to use a move or a copy

    events.emplace_back(EntitySpawnEvent{
            .entity_id = player.id,
            .entity_type = EntityType::PLAYER,
            .entity_pos = player.pos,
            .entity_dir = player.dir,
            .entity_name = player.username,
            .entity_race = player.race,
            .entity_class = player.class_type,
    });

    return events;
}

std::vector<ServerEvent> Game::process_command(const ClientCommand& cmd) {
    if (std::holds_alternative<MoveCmd>(cmd)) {
        return handle_move(std::get<MoveCmd>(cmd));
    }
    return {};
}

std::vector<ServerEvent> Game::tick(uint32_t delta_ms) { return {}; }

std::vector<ServerEvent> Game::handle_move(const MoveCmd& cmd) {
    player.dir = cmd.direction;

    int16_t dx = 0;
    int16_t dy = 0;
    switch (cmd.direction) {
        case Direction::NORTH:
            dy = -4;
            break;
        case Direction::SOUTH:
            dy = 4;
            break;
        case Direction::WEST:
            dx = -4;
            break;
        case Direction::EAST:
            dx = 4;
            break;
    }

    int16_t new_x = static_cast<int16_t>(player.pos.x) + dx;
    int16_t new_y = static_cast<int16_t>(player.pos.y) + dy;

    new_x = std::max<int16_t>(0, std::min<int16_t>(new_x, static_cast<int16_t>(world_w - 60)));
    new_y = std::max<int16_t>(0, std::min<int16_t>(new_y, static_cast<int16_t>(world_h - 160)));

    player.pos.x = static_cast<uint16_t>(new_x);
    player.pos.y = static_cast<uint16_t>(new_y);

    std::vector<ServerEvent> events;
    events.emplace_back(EntityMoveEvent{
            .entity_id = player.id,
            .entity_pos = player.pos,
            .entity_dir = player.dir,
    });
    return events;
}
