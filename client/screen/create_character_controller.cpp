#include "create_character_controller.h"

static const Race RACES[4] = {Race::HUMAN, Race::ELF, Race::DWARF, Race::GNOME};
static const PlayerClass CLASSES[4] = {PlayerClass::MAGE, PlayerClass::CLERIC, PlayerClass::PALADIN,
                                        PlayerClass::WARRIOR};

CreateCharacterController::CreateCharacterController(SDL2pp::Renderer& sdl_renderer,
                                                     const UIConfig& ui_cfg):
        renderer(sdl_renderer, ui_cfg, username, password) {
    username.set_focus(true);
}

void CreateCharacterController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            submitted = true;
            return;
        }
        if (event.key.keysym.sym == SDLK_TAB) {
            active_field = (active_field + 1) % 2;
            username.set_focus(active_field == 0);
            password.set_focus(active_field == 1);
            return;
        }
    }

    if (active_field == 0)
        username.consume_event(event);
    else
        password.consume_event(event);

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int x = event.button.x;
        const int y = event.button.y;

        if (renderer.is_username_hit(x, y)) {
            active_field = 0;
            username.set_focus(true);
            password.set_focus(false);
        } else if (renderer.is_password_hit(x, y)) {
            active_field = 1;
            username.set_focus(false);
            password.set_focus(true);
        } else if (renderer.is_create_button_hit(x, y)) {
            submitted = true;
        } else if (renderer.is_back_button_hit(x, y)) {
            back_requested = true;
        } else {
            const int r = renderer.race_hit(x, y);
            if (r >= 0)
                race = RACES[r];
            const int c = renderer.class_hit(x, y);
            if (c >= 0)
                player_class = CLASSES[c];
        }
    }
}

void CreateCharacterController::handle_mouse_motion(int x, int y) {
    renderer.set_create_button_hovered(x, y);
    renderer.set_back_button_hovered(x, y);
}

void CreateCharacterController::render() { renderer.render(race, player_class); }

void CreateCharacterController::reset_fields() {
    username.clear();
    password.clear();
}

void CreateCharacterController::set_error(const std::string& msg) { renderer.set_error(msg); }
void CreateCharacterController::clear_error() { renderer.clear_error(); }
