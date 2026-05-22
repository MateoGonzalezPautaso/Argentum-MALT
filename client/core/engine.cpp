#include "engine.h"

Engine::Engine(const ClientConfig& config, Queue<ClientCommand>& command_queue):
        render_ctx(config.window.title, config.window.width, config.window.height,
                   config.viewport.logical_w, config.viewport.logical_h),
        menu_ctrl(render_ctx.renderer(), config.ui),
        login_ctrl(render_ctx.renderer(), config.ui),
        game_controller(render_ctx.renderer(), config, command_queue) {}

bool Engine::dispatch_event(GameState& state) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }

        switch (state) {
            case GameState::Menu:
                if (!dispatch_menu_event(event, state))
                    return false;
                break;
            case GameState::Login:
                dispatch_login_event(event, state);
                break;
            case GameState::Playing:
                if (!game_controller.handle_event(event))
                    return false;
                break;
        }
    }
    return true;
}

bool Engine::dispatch_menu_event(const SDL_Event& event, GameState& state) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        return false;
    }
    if (event.type == SDL_MOUSEMOTION) {
        menu_ctrl.handle_mouse_motion(event.motion.x, event.motion.y);
        return true;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (menu_ctrl.is_start_hit(event.button.x, event.button.y)) {
            state = GameState::Login;
        }
        return true;
    }
    return true;
}

void Engine::dispatch_login_event(const SDL_Event& event, GameState& state) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        state = GameState::Menu;
        return;
    }
    if (event.type == SDL_MOUSEMOTION) {
        login_ctrl.handle_mouse_motion(event.motion.x, event.motion.y);
        return;
    }
    login_ctrl.handle_event(event);
}

void Engine::render_menu_frame() {
    render_ctx.renderer().SetDrawColor(0, 0, 0, 255);
    render_ctx.renderer().Clear();
    menu_ctrl.render();
    render_ctx.renderer().Present();
}

void Engine::render_login_frame() {
    render_ctx.renderer().SetDrawColor(0, 0, 0, 255);
    render_ctx.renderer().Clear();
    login_ctrl.render();
    render_ctx.renderer().Present();
}

void Engine::render_game_frame() { game_controller.render(); }

void Engine::tick_game() { game_controller.tick(); }

void Engine::apply_server_event(const ServerEvent& ev) { game_controller.apply_server_event(ev); }

bool Engine::try_submit_login(std::string& username, std::string& password) {
    if (!login_ctrl.is_submitted()) {
        return false;
    }
    login_ctrl.clear_error();
    username = login_ctrl.username_text();
    password = login_ctrl.password_text();
    login_ctrl.reset_submitted();
    return true;
}

void Engine::handle_login_error(const std::string& msg) {
    login_ctrl.reset_fields();
    login_ctrl.set_error(msg);
}

void Engine::reset_login_state() { login_ctrl.clear_error(); }
