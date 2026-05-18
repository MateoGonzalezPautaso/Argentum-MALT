#include "move_controller.h"

#include <algorithm>
#include <cmath>

#include "client_protocol.h"
#include "renderer.h"

MoveController::MoveController(ClientRenderer& renderer, ClientProtocol& protocol,
                               const MoveConfig& config, uint32_t initial_ticks):
        renderer(renderer),
        protocol(protocol),
        config(config),
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
    switch (dir) {
        case Direction::WEST:
            apply_movement(dir, -config.move_step, 0, now, true);
            break;
        case Direction::EAST:
            apply_movement(dir, config.move_step, 0, now, true);
            break;
        case Direction::NORTH:
            apply_movement(dir, 0, -config.move_step, now, true);
            break;
        case Direction::SOUTH:
            apply_movement(dir, 0, config.move_step, now, true);
            break;
    }
}

void MoveController::apply_movement(Direction dir, int dx, int dy, uint32_t now,
                                    bool cancel_target) {
    if (cancel_target) {
        has_target = false;
    }

    set_direction_rows(dir);
    advance_walk_frame(dir, now);

    renderer.move_sprite(dx, dy);
    protocol.send_command(MoveCmd{dir});
}

void MoveController::set_direction_rows(Direction dir) {
    if (dir == Direction::NORTH) {
        renderer.set_movable_src_y(config.dir_src_y_up);
        renderer.set_anchor_src_y(config.head_dir_src_y_up);
    } else if (dir == Direction::SOUTH) {
        renderer.set_movable_src_y(config.dir_src_y_down);
        renderer.set_anchor_src_y(config.head_dir_src_y_down);
    } else if (dir == Direction::WEST) {
        renderer.set_movable_src_y(config.dir_src_y_left);
        renderer.set_anchor_src_y(config.head_dir_src_y_left);
    } else if (dir == Direction::EAST) {
        renderer.set_movable_src_y(config.dir_src_y_right);
        renderer.set_anchor_src_y(config.head_dir_src_y_right);
    }
}

void MoveController::advance_walk_frame(Direction dir, uint32_t now) {
    if (now - last_walk_tick < config.walk_frame_ms) {
        return;
    }
    renderer.step_movable_src_x(config.walk_src_step, walk_src_frames_for(dir));
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
    if (!renderer.get_movable_position(x, y)) {
        has_target = false;
        return false;
    }
    return true;
}

bool MoveController::should_stop_at_target(int current_x, int current_y, int new_x, int new_y) {
    if (new_x == target_x && new_y == target_y) {
        return true;
    }
    if (new_x == current_x && new_y == current_y) {
        return true;
    }
    return false;
}

void MoveController::compute_step_to_target(int current_x, int current_y, int& move_dx, int& move_dy,
                                            Direction& dir) const {
    const int dx = target_x - current_x;
    const int dy = target_y - current_y;
    move_dx = 0;
    move_dy = 0;
    dir = Direction::SOUTH;

    if (std::abs(dx) >= std::abs(dy)) {
        if (dx > 0) {
            move_dx = std::min(config.move_step, dx);
            dir = Direction::EAST;
        } else {
            move_dx = std::max(-config.move_step, dx);
            dir = Direction::WEST;
        }
    } else {
        if (dy > 0) {
            move_dy = std::min(config.move_step, dy);
            dir = Direction::SOUTH;
        } else {
            move_dy = std::max(-config.move_step, dy);
            dir = Direction::NORTH;
        }
    }
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

    int move_dx = 0;
    int move_dy = 0;
    Direction dir = Direction::SOUTH;
    compute_step_to_target(current_x, current_y, move_dx, move_dy, dir);
    apply_movement(dir, move_dx, move_dy, now, false);

    int new_x = 0;
    int new_y = 0;
    if (!get_movable_position(new_x, new_y)) {
        return;
    }

    if (should_stop_at_target(current_x, current_y, new_x, new_y)) {
        has_target = false;
    }
}
