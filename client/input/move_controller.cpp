#include "move_controller.h"

#include <cmath>

#include "../config/config.h"

MoveConfig::MoveConfig(const ClientConfig& config):
        move_step(config.move_step),
        walk_src_step(config.walk_src_step),
        walk_frame_ms(config.walk_frame_ms) {
    dir_data[Direction::NORTH] = {config.dir_src_y_up, config.head_dir_src_y_up,
                                   config.walk_src_frames_up};
    dir_data[Direction::SOUTH] = {config.dir_src_y_down, config.head_dir_src_y_down,
                                   config.walk_src_frames_down};
    dir_data[Direction::EAST] = {config.dir_src_y_right, config.head_dir_src_y_right,
                                  config.walk_src_frames_right};
    dir_data[Direction::WEST] = {config.dir_src_y_left, config.head_dir_src_y_left,
                                  config.walk_src_frames_left};
}

MoveController::MoveController(Queue<ClientCommand>& command_queue, const MoveConfig& config,
                               uint32_t initial_ticks):
        command_queue(command_queue),
        config(config),
        last_walk_tick(initial_ticks) {}

std::optional<Direction> MoveController::tick(uint32_t now) {
    if (!has_target) {
        return std::nullopt;
    }
    return move_toward_target(now);
}

std::optional<Direction> MoveController::move_direction(Direction dir, uint32_t now) {
    current_dir_ = dir;
    if (!can_walk(now)) {
        return std::nullopt;
    }
    last_walk_tick = now;
    command_queue.push(MoveCmd{dir});
    return dir;
}

void MoveController::set_move_target(int x, int y) {
    target_x = x;
    target_y = y;
    has_target = true;
}

void MoveController::set_position(int x, int y) {
    pos_x = x;
    pos_y = y;
}

bool MoveController::can_walk(uint32_t now) {
    return now - last_walk_tick >= config.walk_frame_ms;
}

Direction MoveController::compute_direction_to_target(int current_x, int current_y) const {
    const int dx = target_x - current_x;
    const int dy = target_y - current_y;

    if (std::abs(dx) >= std::abs(dy)) {
        return dx > 0 ? Direction::EAST : Direction::WEST;
    }
    return dy > 0 ? Direction::SOUTH : Direction::NORTH;
}

std::optional<Direction> MoveController::move_toward_target(uint32_t now) {
    if (std::abs(target_x - pos_x) < config.move_step &&
        std::abs(target_y - pos_y) < config.move_step) {
        has_target = false;
        return std::nullopt;
    }

    Direction dir = compute_direction_to_target(pos_x, pos_y);
    return move_direction(dir, now);
}
