#ifndef CLIENT_CREATE_CHARACTER_CONTROLLER_H
#define CLIENT_CREATE_CHARACTER_CONTROLLER_H

#include <string>

#include <SDL2/SDL.h>

#include "../../../common/messages.h"
#include "../../config/config.h"
#include "../screens/create_character_renderer.h"
#include "credential_form.h"

class CreateCharacterController {
public:
    CreateCharacterController(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg);

    void handle_event(const SDL_Event& event);
    void handle_mouse_motion(int x, int y);
    void render();

    bool is_submitted() const { return submitted; }
    bool is_back_requested() const { return back_requested; }
    void reset_submitted() { submitted = false; }
    void reset_back() { back_requested = false; }
    void reset_fields();
    void set_error(const std::string& msg);
    void clear_error();

    const std::string& username_text() const { return form_.first_value(); }
    const std::string& password_text() const { return form_.second_value(); }
    Race selected_race() const { return race; }
    PlayerClass selected_class() const { return player_class; }

private:
    CredentialForm form_;
    Race race = Race::HUMAN;
    PlayerClass player_class = PlayerClass::WARRIOR;
    bool submitted = false;
    bool back_requested = false;
    CreateCharacterRenderer renderer;
};

#endif
