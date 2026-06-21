#include "login_controller.h"

LoginController::LoginController(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg):
        login_renderer(renderer, ui_cfg, form_.first_field(), form_.second_field()) {}

void LoginController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (form_.handle_keydown(event)) {
            login_submitted = true;
            return;
        }
        // TAB ya fue procesado dentro de handle_keydown; si fue TAB no llegamos acá.
        if (event.key.keysym.sym == SDLK_TAB)
            return;
    }

    form_.dispatch_to_active(event);

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int x = event.button.x;
        const int y = event.button.y;

        if (login_renderer.is_username_hit(x, y)) {
            form_.focus_first();
        } else if (login_renderer.is_password_hit(x, y)) {
            form_.focus_second();
        } else if (login_renderer.is_connect_button_hit(x, y)) {
            login_submitted = true;
        } else if (login_renderer.is_new_account_hit(x, y)) {
            create_char_requested = true;
        } else {
            form_.blur();
        }
    }
}

void LoginController::handle_mouse_motion(int x, int y) {
    login_renderer.set_connect_button_hovered(x, y);
    login_renderer.set_audio_button_hovered(x, y);
    login_renderer.set_new_account_button_hovered(x, y);
}

bool LoginController::is_audio_hit(int x, int y) const {
    return login_renderer.is_audio_hit(x, y);
}

void LoginController::set_audio_muted(bool muted) { login_renderer.set_audio_muted(muted); }

void LoginController::reset_fields() { form_.reset(); }

void LoginController::set_error(const std::string& msg) { login_renderer.set_error(msg); }

void LoginController::clear_error() { login_renderer.clear_error(); }

const std::string& LoginController::username_text() const { return form_.first_value(); }

const std::string& LoginController::password_text() const { return form_.second_value(); }

void LoginController::render() { login_renderer.render(); }
