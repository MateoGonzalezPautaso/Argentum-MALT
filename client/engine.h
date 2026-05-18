#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <string>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "chat_input.h"
#include "client_protocol.h"
#include "config.h"
#include "renderer.h"

class ClientEngine {
private:
    // objetos de SDL que viven durante toda la vida del engine.
    SDL2pp::SDL sdl;
    SDL2pp::Window window;
    ClientRenderer renderer;
    ClientProtocol& protocol;
    uint32_t last_walk_tick = 0;
    uint32_t walk_frame_ms = 120;
    int move_step = 4;
    int walk_src_step = 27;
    int walk_src_frames = 6;
    int walk_src_frames_down = 6;
    int walk_src_frames_up = 6;
    int walk_src_frames_left = 6;
    int walk_src_frames_right = 6;
    int dir_src_y_down = 0;
    int dir_src_y_up = 40;
    int dir_src_y_left = 80;
    int dir_src_y_right = 120;
    int head_dir_src_y_down = 0;
    int head_dir_src_y_up = 64;
    int head_dir_src_y_left = 128;
    int head_dir_src_y_right = 192;
    bool has_target = false;
    int target_x = 0;
    int target_y = 0;
    ChatInput chat_input;

public:
    explicit ClientEngine(const ClientConfig& config, ClientProtocol& protocol);
    ~ClientEngine();

    void tick();
    void show_menu();
    void show_sprite();
    void move_sprite(int dx, int dy);
    bool handle_event(const SDL_Event& event);
    bool is_menu_click(int x, int y) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);

private:
    bool handle_mouse_button(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
    void set_direction_rows(Direction dir);
    void advance_walk_frame(Direction dir, uint32_t now);
    void apply_movement(Direction dir, int dx, int dy, uint32_t now, bool cancel_target);
    bool get_movable_position(int& x, int& y);
    bool should_stop_at_target(int current_x, int current_y, int new_x, int new_y);
    void compute_step_to_target(int current_x, int current_y, int& move_dx, int& move_dy,
                                Direction& dir) const;
    void set_move_target(int x, int y);
    void move_toward_target(uint32_t now);
    int walk_src_frames_for(Direction dir) const;
    void sync_chat_to_renderer();
};

#endif
