#ifndef MOVE_CONTROLLER_H
#define MOVE_CONTROLLER_H

#include <cstdint>

#include "../common/messages.h"

struct ClientConfig;
class ClientProtocol;
class WorldRenderer;

struct MoveConfig {
    int move_step = 4;
    int walk_src_step = 27;
    int walk_src_frames = 6;
    int walk_src_frames_down = 6;
    int walk_src_frames_up = 6;
    int walk_src_frames_left = 6;
    int walk_src_frames_right = 6;
    uint32_t walk_frame_ms = 120;
    int dir_src_y_down = 0;
    int dir_src_y_up = 40;
    int dir_src_y_left = 80;
    int dir_src_y_right = 120;
    int head_dir_src_y_down = 0;
    int head_dir_src_y_up = 64;
    int head_dir_src_y_left = 128;
    int head_dir_src_y_right = 192;

    MoveConfig() = default;
    explicit MoveConfig(const ClientConfig& config);
};

class MoveController {
private:
    WorldRenderer& world_renderer;
    ClientProtocol& protocol;
    MoveConfig config;
    uint32_t last_walk_tick = 0;
    bool has_target = false;
    int target_x = 0;
    int target_y = 0;

public:
    MoveController(WorldRenderer& world_renderer, ClientProtocol& protocol,
                   const MoveConfig& config, uint32_t initial_ticks);

    void tick(uint32_t now);
    void set_move_target(int x, int y);
    void move_direction(Direction dir, uint32_t now);

private:
    void set_direction_rows(Direction dir);
    void advance_walk_frame(Direction dir, uint32_t now);
    void apply_movement(Direction dir, uint32_t now, bool cancel_target);
    bool get_movable_position(int& x, int& y);
    Direction compute_direction_to_target(int current_x, int current_y) const;
    void move_toward_target(uint32_t now);
    int walk_src_frames_for(Direction dir) const;
};

#endif  // MOVE_CONTROLLER_H
