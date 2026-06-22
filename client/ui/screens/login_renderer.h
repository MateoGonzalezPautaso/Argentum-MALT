#ifndef CLIENT_LOGIN_RENDERER_H
#define CLIENT_LOGIN_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>

#include "../../config/config.h"
#include "../../render/gfx/button.h"
#include "credential_screen_renderer.h"

class ChatInput;

class LoginRenderer : public CredentialScreenRenderer {
public:
    LoginRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                  const ChatInput& username_model, const ChatInput& password_model);

    void render();

    bool is_connect_button_hit(int x, int y) const;
    bool is_audio_hit(int x, int y) const;
    bool is_new_account_hit(int x, int y) const;

    void set_connect_button_hovered(int x, int y);
    void set_audio_button_hovered(int x, int y);
    void set_new_account_button_hovered(int x, int y);
    void set_audio_muted(bool muted) { audio_muted_ = muted; }

private:
    Button connect_button;
    Button audio_button;
    SDL2pp::Texture audio_off_texture;
    bool audio_muted_ = false;
    Button new_account_button;

    void init_layout();
};

#endif  // CLIENT_LOGIN_RENDERER_H
