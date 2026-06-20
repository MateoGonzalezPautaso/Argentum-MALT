#ifndef CLIENT_LOGIN_CONTROLLER_H
#define CLIENT_LOGIN_CONTROLLER_H

#include <string>

#include <SDL2/SDL.h>

#include "../config/config.h"
#include "../input/chat_input.h"
#include "../render/login_renderer.h"

class LoginController {
public:
    LoginController(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg);

    void handle_event(const SDL_Event& event);
    void handle_mouse_motion(int x, int y);

    bool is_submitted() const { return login_submitted; }
    void reset_submitted() { login_submitted = false; }
    void reset_fields();
    void set_error(const std::string& msg);
    void clear_error();
    const std::string& username_text() const;
    const std::string& password_text() const;

    bool wants_create_character() const { return create_char_requested; }
    void reset_create_char_request() { create_char_requested = false; }

    bool is_audio_hit(int x, int y) const;
    void set_audio_muted(bool muted);

    void render();

private:
    ChatInput login_username;
    ChatInput login_password;
    int login_active_field = 0;
    bool login_submitted = false;
    bool create_char_requested = false;
    LoginRenderer login_renderer;
};

#endif
