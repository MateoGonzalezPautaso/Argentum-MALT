#include "create_character_renderer.h"

#include <algorithm>

#include "../../input/chat_input.h"
#include "../../render/gfx/geometry.h"
#include "../../render/gfx/text_renderer.h"
#include "../../render/gfx/texture_loader.h"

CreateCharacterRenderer::CreateCharacterRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                                                 const ChatInput& username_model,
                                                 const ChatInput& password_model):
        CredentialScreenRenderer(renderer, ui_cfg, username_model, password_model,
                                 "Create Character"),
        create_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_hover))),
        back_button(SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_back_default)),
                    SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_back_hover))) {
    init_layout();
}

void CreateCharacterRenderer::init_layout() {
    const int sel_w = ui_cfg.login_field_w / 4;
    const int sel_h = ui_cfg.login_field_h;
    const int sel_x = password_field_rect.GetX();
    const int password_y = password_field_rect.GetY();

    const int race_y = password_y + ui_cfg.login_field_h + ui_cfg.login_field_gap * 2;
    for (int i = 0; i < 4; ++i)
        race_rects[i] = SDL2pp::Rect(sel_x + i * sel_w, race_y, sel_w, sel_h);

    const int class_y = race_y + sel_h + ui_cfg.login_field_gap;
    for (int i = 0; i < 4; ++i)
        class_rects[i] = SDL2pp::Rect(sel_x + i * sel_w, class_y, sel_w, sel_h);

    const int btn_w = create_button.default_tex.GetWidth();
    const int btn_h = create_button.default_tex.GetHeight();
    const int btn_x = std::max(0, (ui_cfg.window_w - btn_w) / 2);
    const int btn_y = class_y + sel_h + ui_cfg.login_button_spacing;
    create_button.set_position(btn_x, btn_y, btn_w, btn_h);

    const int back_w = back_button.default_tex.GetWidth();
    const int back_h = back_button.default_tex.GetHeight();
    back_button.set_position(ui_cfg.back_button_x, ui_cfg.back_button_y, back_w, back_h);
}

void CreateCharacterRenderer::render(Race selected_race, PlayerClass selected_class) {
    render_chrome();

    const int race_idx = static_cast<int>(selected_race) - 1;
    std::array<std::string, 4> race_labels = {"Human", "Elf", "Dwarf", "Gnome"};
    render_selector(race_rects, race_labels, race_idx);

    const int class_idx = static_cast<int>(selected_class) - 1;
    std::array<std::string, 4> class_labels = {"Mage", "Cleric", "Paladin", "Warrior"};
    render_selector(class_rects, class_labels, class_idx);

    create_button.render(renderer);
    back_button.render(renderer);
    render_error(create_button.rect.GetY() + create_button.rect.GetH());
    renderer.Present();
}

void CreateCharacterRenderer::render_selector(const std::array<SDL2pp::Rect, 4>& rects,
                                              const std::array<std::string, 4>& labels,
                                              int selected_idx) const {
    for (int i = 0; i < 4; ++i) {
        const bool sel = (i == selected_idx);
        renderer.SetDrawColor(sel ? SDL_Color{60, 55, 20, 220} : SDL_Color{30, 30, 30, 220});
        renderer.FillRect(rects[i]);
        renderer.SetDrawColor(sel ? selected_color : unselected_color);
        renderer.DrawRect(rects[i]);

        if (field_font) {
            SDL_Color color = sel ? selected_color : text_color;
            texture::render_text_clipped(renderer, field_font, labels[i], color, rects[i], 4, 2,
                                         true);
        }
    }
}

bool CreateCharacterRenderer::is_create_button_hit(int x, int y) const {
    return create_button.is_hit(x, y);
}
bool CreateCharacterRenderer::is_back_button_hit(int x, int y) const {
    return back_button.is_hit(x, y);
}

int CreateCharacterRenderer::race_at(int x, int y) const {
    for (int i = 0; i < 4; ++i)
        if (point_in_rect(x, y, race_rects[i]))
            return i;
    return -1;
}

int CreateCharacterRenderer::class_at(int x, int y) const {
    for (int i = 0; i < 4; ++i)
        if (point_in_rect(x, y, class_rects[i]))
            return i;
    return -1;
}

void CreateCharacterRenderer::set_create_button_hovered(int x, int y) {
    create_button.hovered = create_button.is_hit(x, y);
}
void CreateCharacterRenderer::set_back_button_hovered(int x, int y) {
    back_button.hovered = back_button.is_hit(x, y);
}
