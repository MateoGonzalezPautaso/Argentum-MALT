#include "engine.h"

Engine::Engine(const ClientConfig& config, Queue<ClientCommand>& command_queue,
               AudioManager& audio_manager):
        render_ctx(config.window.title, config.window.width, config.window.height,
                   config.viewport.logical_w, config.viewport.logical_h, config.window.fullscreen),
        menu_ctrl(render_ctx.renderer(), config.ui),
        login_ctrl(render_ctx.renderer(), config.ui),
        create_char_ctrl(render_ctx.renderer(), config.ui),
        audio_manager(audio_manager),
        game_controller(render_ctx.renderer(), config, command_queue, audio_manager) {}

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
            case GameState::CreateCharacter:
                dispatch_create_char_event(event, state);
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
            login_ctrl.set_audio_muted(audio_manager.is_muted());
        } else if (menu_ctrl.is_audio_hit(event.button.x, event.button.y)) {
            bool muted = !audio_manager.is_muted();
            audio_manager.set_muted(muted);
            menu_ctrl.set_audio_muted(muted);
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
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (login_ctrl.is_audio_hit(event.button.x, event.button.y)) {
            bool muted = !audio_manager.is_muted();
            audio_manager.set_muted(muted);
            menu_ctrl.set_audio_muted(muted);
            login_ctrl.set_audio_muted(muted);
            return;
        }
    }
    login_ctrl.handle_event(event);
    if (login_ctrl.wants_create_character()) {
        create_char_requested = true;
        login_ctrl.reset_create_char_request();
        state = GameState::CreateCharacter;
    }
}

void Engine::dispatch_create_char_event(const SDL_Event& event, GameState& state) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        state = GameState::Login;
        return;
    }
    if (event.type == SDL_MOUSEMOTION) {
        create_char_ctrl.handle_mouse_motion(event.motion.x, event.motion.y);
        return;
    }
    create_char_ctrl.handle_event(event);
    if (create_char_ctrl.is_back_requested()) {
        create_char_ctrl.reset_back();
        state = GameState::Login;
    }
}

void Engine::render_menu_frame() { menu_ctrl.render(); }
void Engine::render_login_frame() { login_ctrl.render(); }
void Engine::render_create_char_frame() { create_char_ctrl.render(); }
void Engine::render_game_frame() { game_controller.render(); }

void Engine::tick_game() { game_controller.tick(); }

void Engine::apply_server_event(const ServerEvent& ev) { game_controller.apply_server_event(ev); }

void Engine::load_game_assets() { game_controller.load_game_assets(); }

bool Engine::is_world_map_loaded() const { return game_controller.is_world_map_loaded(); }

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

bool Engine::try_submit_create_character(std::string& username, std::string& password, Race& race,
                                         PlayerClass& player_class) {
    if (!create_char_ctrl.is_submitted())
        return false;
    create_char_ctrl.clear_error();
    username = create_char_ctrl.username_text();
    password = create_char_ctrl.password_text();
    race = create_char_ctrl.selected_race();
    player_class = create_char_ctrl.selected_class();
    create_char_ctrl.reset_submitted();
    return true;
}

void Engine::handle_create_char_error(const std::string& msg) {
    create_char_ctrl.reset_fields();
    create_char_ctrl.set_error(msg);
}

void Engine::reset_create_char_state() {
    create_char_ctrl.clear_error();
    create_char_requested = false;
}

bool Engine::wants_create_character() const { return create_char_requested; }
