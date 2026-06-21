#include "create_character_controller.h"

#include <string>

static const Race RACES[4] = {Race::HUMAN, Race::ELF, Race::DWARF, Race::GNOME};
static const PlayerClass CLASSES[4] = {PlayerClass::MAGE, PlayerClass::CLERIC, PlayerClass::PALADIN,
                                       PlayerClass::WARRIOR};

CreateCharacterController::CreateCharacterController(SDL2pp::Renderer& sdl_renderer,
                                                     const UIConfig& ui_cfg):
        renderer(sdl_renderer, ui_cfg, form_.first_field(), form_.second_field()) {}

void CreateCharacterController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (form_.handle_keydown(event)) {
            submitted = true;
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

        if (renderer.is_username_hit(x, y)) {
            form_.focus_first();
        } else if (renderer.is_password_hit(x, y)) {
            form_.focus_second();
        } else if (renderer.is_create_button_hit(x, y)) {
            submitted = true;
        } else if (renderer.is_back_button_hit(x, y)) {
            back_requested = true;
        } else {
            // Selección de raza y clase: específico de CreateCharacterController.
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

void CreateCharacterController::reset_fields() { form_.reset(); }

void CreateCharacterController::set_error(const std::string& msg) { renderer.set_error(msg); }
void CreateCharacterController::clear_error() { renderer.clear_error(); }
