#include "move_controller.h"

#include <cmath>

#include "client_protocol.h"
#include "world_renderer.h"

MoveController::MoveController(WorldRenderer& world_renderer, ClientProtocol& protocol,
                               const MoveConfig& config, uint32_t initial_ticks):
        world_renderer(world_renderer), protocol(protocol), config(config),
        last_walk_tick(initial_ticks) {}

void MoveController::tick(uint32_t now) {
    if (!has_target) {
        return;
    }
    move_toward_target(now);
}

void MoveController::set_move_target(int x, int y) {
    target_x = x;
    target_y = y;
    has_target = true;
}

void MoveController::move_direction(Direction dir, uint32_t now) {
    apply_movement(dir, now, true);
}

void MoveController::apply_movement(Direction dir, uint32_t now, bool cancel_target) {
    if (cancel_target) {
        has_target = false;
    }

    set_direction_rows(dir);
    advance_walk_frame(dir, now);
    protocol.send_command(MoveCmd{dir});
}

void MoveController::set_direction_rows(Direction dir) {
    if (dir == Direction::NORTH) {
        world_renderer.set_movable_src_y(config.dir_src_y_up);
        world_renderer.set_anchor_src_y(config.head_dir_src_y_up);
    } else if (dir == Direction::SOUTH) {
        world_renderer.set_movable_src_y(config.dir_src_y_down);
        world_renderer.set_anchor_src_y(config.head_dir_src_y_down);
    } else if (dir == Direction::WEST) {
        world_renderer.set_movable_src_y(config.dir_src_y_left);
        world_renderer.set_anchor_src_y(config.head_dir_src_y_left);
    } else if (dir == Direction::EAST) {
        world_renderer.set_movable_src_y(config.dir_src_y_right);
        world_renderer.set_anchor_src_y(config.head_dir_src_y_right);
    }
}

void MoveController::advance_walk_frame(Direction dir, uint32_t now) {
    if (now - last_walk_tick < config.walk_frame_ms) {
        return;
    }
    world_renderer.step_movable_src_x(config.walk_src_step, walk_src_frames_for(dir));
    last_walk_tick = now;
}

int MoveController::walk_src_frames_for(Direction dir) const {
    switch (dir) {
        case Direction::NORTH:
            return config.walk_src_frames_up;
        case Direction::SOUTH:
            return config.walk_src_frames_down;
        case Direction::WEST:
            return config.walk_src_frames_left;
        case Direction::EAST:
            return config.walk_src_frames_right;
        default:
            return config.walk_src_frames;
    }
}

bool MoveController::get_movable_position(int& x, int& y) {
    if (!world_renderer.get_movable_position(x, y)) {
        has_target = false;
        return false;
    }
    return true;
}

Direction MoveController::compute_direction_to_target(int current_x, int current_y) const {
    const int dx = target_x - current_x;
    const int dy = target_y - current_y;

    if (std::abs(dx) >= std::abs(dy)) {
        return dx > 0 ? Direction::EAST : Direction::WEST;
    }
    return dy > 0 ? Direction::SOUTH : Direction::NORTH;
}

void MoveController::move_toward_target(uint32_t now) {
    int current_x = 0;
    int current_y = 0;
    if (!get_movable_position(current_x, current_y)) {
        return;
    }

    if (target_x == current_x && target_y == current_y) {
        has_target = false;
        return;
    }

    Direction dir = compute_direction_to_target(current_x, current_y);
    apply_movement(dir, now, false);
}
