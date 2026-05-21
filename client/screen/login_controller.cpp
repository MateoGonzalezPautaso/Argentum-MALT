#include "login_controller.h"

LoginController::LoginController(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg):
        login_renderer(renderer, ui_cfg, login_username, login_password) {
    login_username.set_focus(true);
}

void LoginController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            login_submitted = true;
            return;
        }
        if (event.key.keysym.sym == SDLK_TAB) {
            login_active_field = (login_active_field + 1) % 2;
            login_username.set_focus(login_active_field == 0);
            login_password.set_focus(login_active_field == 1);
            return;
        }
    }

    if (login_active_field == 0) {
        login_username.consume_event(event);
    } else {
        login_password.consume_event(event);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int x = event.button.x;
        const int y = event.button.y;

        if (login_renderer.is_username_hit(x, y)) {
            login_active_field = 0;
            login_username.set_focus(true);
            login_password.set_focus(false);
        } else if (login_renderer.is_password_hit(x, y)) {
            login_active_field = 1;
            login_username.set_focus(false);
            login_password.set_focus(true);
        } else if (login_renderer.is_connect_button_hit(x, y)) {
            login_submitted = true;
        } else {
            login_username.set_focus(false);
            login_password.set_focus(false);
        }
    }
}

void LoginController::handle_mouse_motion(int x, int y) {
    login_renderer.set_connect_button_hovered(x, y);
    login_renderer.set_back_button_hovered(x, y);
}

void LoginController::reset_fields() {
    login_username.clear();
    login_password.clear();
}

void LoginController::set_error(const std::string& msg) { login_renderer.set_error(msg); }

void LoginController::clear_error() { login_renderer.clear_error(); }

const std::string& LoginController::username_text() const { return login_username.get_text(); }

const std::string& LoginController::password_text() const { return login_password.get_text(); }

void LoginController::render() { login_renderer.render(); }
