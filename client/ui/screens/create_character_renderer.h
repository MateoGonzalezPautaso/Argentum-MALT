#ifndef CLIENT_CREATE_CHARACTER_RENDERER_H
#define CLIENT_CREATE_CHARACTER_RENDERER_H

#include <array>
#include <string>

#include <SDL2pp/SDL2pp.hh>

#include "../../../common/messages.h"
#include "../../config/config.h"
#include "../../render/gfx/button.h"

#include "credential_screen_renderer.h"

class ChatInput;

class CreateCharacterRenderer: public CredentialScreenRenderer {
public:
    CreateCharacterRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                            const ChatInput& username_model, const ChatInput& password_model);

    void render(Race selected_race, PlayerClass selected_class);

    bool is_create_button_hit(int x, int y) const;
    bool is_back_button_hit(int x, int y) const;
    int race_at(int x, int y) const;
    int class_at(int x, int y) const;

    void set_create_button_hovered(int x, int y);
    void set_back_button_hovered(int x, int y);

private:
    SDL_Color selected_color{200, 180, 80, 255};
    SDL_Color unselected_color{100, 100, 100, 255};

    Button create_button;
    Button back_button;

    std::array<SDL2pp::Rect, 4> race_rects;
    std::array<SDL2pp::Rect, 4> class_rects;

    void init_layout();
    void render_selector(const std::array<SDL2pp::Rect, 4>& rects,
                         const std::array<std::string, 4>& labels, int selected_idx) const;
};

#endif  // CLIENT_CREATE_CHARACTER_RENDERER_H
