#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "config.h"
#include "client_protocol.h"
#include "renderer.h"

class ClientEngine {
private:
    SDL2pp::SDL sdl;
    SDL2pp::Window window;
    ClientRenderer renderer;
    ClientProtocol& protocol;
    uint32_t last_walk_tick = 0;
    uint32_t walk_frame_ms = 120;
    int move_step = 4;
    int walk_src_step = 30;
    int walk_src_max = 60;
    int dir_src_y_down = 0;
    int dir_src_y_up = 40;
    int dir_src_y_left = 80;
    int dir_src_y_right = 120;

public:
    explicit ClientEngine(const ClientConfig& config, ClientProtocol& protocol);

    void show_sprite();
    void move_sprite(int dx, int dy);
    bool handle_event(const SDL_Event& event);
    void toggle_extra_sprite();
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int max_x);
};

#endif
