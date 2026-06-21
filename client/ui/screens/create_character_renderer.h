#ifndef CLIENT_CREATE_CHARACTER_RENDERER_H
#define CLIENT_CREATE_CHARACTER_RENDERER_H

#include <array>
#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../../common/messages.h"
#include "../../config/config.h"

#include "../../render/gfx/button.h"

class ChatInput;

class CreateCharacterRenderer {
public:
    CreateCharacterRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                            const ChatInput& username_model, const ChatInput& password_model);
    ~CreateCharacterRenderer();

    void render(Race selected_race, PlayerClass selected_class);

    bool is_username_hit(int x, int y) const;
    bool is_password_hit(int x, int y) const;
    bool is_create_button_hit(int x, int y) const;
    bool is_back_button_hit(int x, int y) const;
    int race_at(int x, int y) const;
    int class_at(int x, int y) const;

    void set_create_button_hovered(int x, int y);
    void set_back_button_hovered(int x, int y);

    void set_error(const std::string& text);
    void clear_error();

private:
    SDL2pp::Renderer& renderer;
    const ChatInput& username_model;
    const ChatInput& password_model;

    std::string error_text;
    SDL_Color error_color{255, 60, 60, 255};
    SDL_Color text_color{255, 255, 255, 255};
    SDL_Color placeholder_color{160, 160, 160, 255};
    SDL_Color title_color{255, 215, 0, 255};
    SDL_Color selected_color{200, 180, 80, 255};
    SDL_Color unselected_color{100, 100, 100, 255};

    SDL2pp::Texture background_texture;
    SDL2pp::Texture logo_texture;
    Button create_button;
    Button back_button;

    SDL2pp::Rect background_rect;
    SDL2pp::Rect logo_rect;
    SDL2pp::Rect title_rect;
    SDL2pp::Rect username_field_rect;
    SDL2pp::Rect password_field_rect;
    std::array<SDL2pp::Rect, 4> race_rects;
    std::array<SDL2pp::Rect, 4> class_rects;

    TTF_Font* field_font = nullptr;
    TTF_Font* title_font = nullptr;
    UIConfig ui_cfg;

    void init_layout();
    void render_text_field(const SDL2pp::Rect& rect, const std::string& text, bool focused,
                           const std::string& placeholder) const;
    void render_selector(const std::array<SDL2pp::Rect, 4>& rects,
                         const std::array<std::string, 4>& labels, int selected_idx) const;
    void render_error() const;
};

#endif
