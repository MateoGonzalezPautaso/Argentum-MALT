#ifndef MOVE_CONTROLLER_H
#define MOVE_CONTROLLER_H

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "../../common/messages.h"
#include "../../common/queue.h"

struct ClientConfig;

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
    Queue<ClientCommand>& command_queue;
    MoveConfig config;
    uint32_t last_walk_tick = 0;
    bool has_target = false;
    int target_x = 0;
    int target_y = 0;
    int pos_x = 0;
    int pos_y = 0;
    Direction current_dir_ = Direction::SOUTH;

public:
    MoveController(Queue<ClientCommand>& command_queue, const MoveConfig& config,
                   uint32_t initial_ticks);

    std::optional<Direction> tick(uint32_t now);
    std::optional<Direction> move_direction(Direction dir, uint32_t now);
    void set_move_target(int x, int y);
    void set_position(int x, int y);

    int position_x() const { return pos_x; }
    int position_y() const { return pos_y; }
    Direction current_dir() const { return current_dir_; }

private:
    bool can_walk(uint32_t now);
    Direction compute_direction_to_target(int current_x, int current_y) const;
    std::optional<Direction> move_toward_target(uint32_t now);
};

#endif
