#ifndef MOVE_CONTROLLER_H
#define MOVE_CONTROLLER_H

#include <cstdint>
#include <unordered_map>

#include "../../common/messages.h"
#include "../../common/queue.h"

struct ClientConfig;
class WorldRenderer;

struct DirectionData {
    int body_src_y;
    int head_src_y;
    int walk_src_frames;
};

struct MoveConfig {
    int move_step = 4;
    int walk_src_step = 27;
    uint32_t walk_frame_ms = 120;

    std::unordered_map<Direction, DirectionData> dir_data;

    MoveConfig() = default;
    explicit MoveConfig(const ClientConfig& config);

    int body_src_y_for(Direction dir) const { return dir_data.at(dir).body_src_y; }
    int head_src_y_for(Direction dir) const { return dir_data.at(dir).head_src_y; }
    int walk_src_frames_for(Direction dir) const { return dir_data.at(dir).walk_src_frames; }
};

class MoveController {
private:
    WorldRenderer& world_renderer;
    Queue<ClientCommand>& command_queue;
    MoveConfig config;
    uint32_t last_walk_tick = 0;
    bool has_target = false;
    int target_x = 0;
    int target_y = 0;

public:
    MoveController(WorldRenderer& world_renderer, Queue<ClientCommand>& command_queue,
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
};

#endif  // MOVE_CONTROLLER_H
