#include "create_character_renderer.h"

#include <algorithm>

#include "../../input/chat_input.h"

#include "form_widgets.h"
#include "../../render/gfx/geometry.h"
#include "../../render/gfx/text_renderer.h"
#include "../../render/gfx/texture_loader.h"

static constexpr std::array<std::string_view, 4> RACE_LABELS = {"Human", "Elf", "Dwarf", "Gnome"};
static constexpr std::array<std::string_view, 4> CLASS_LABELS = {"Warrior", "Mage", "Cleric",
                                                                 "Paladin"};

CreateCharacterRenderer::CreateCharacterRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                                                 const ChatInput& username_model,
                                                 const ChatInput& password_model):
        renderer(renderer),
        username_model(username_model),
        password_model(password_model),
        background_texture(renderer, texture::load_surface(ui_cfg.asset_login_bg)),
        logo_texture(renderer, texture::load_surface(ui_cfg.asset_login_logo)),
        create_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_hover))),
        back_button(SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_back_default)),
                    SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_back_hover))),
        background_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        ui_cfg(ui_cfg) {
    field_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_field_size);
    if (!field_font)
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    title_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_title_size);
    if (!title_font)
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    init_layout();
    form_widgets_.emplace(renderer, field_font, ui_cfg);
}

CreateCharacterRenderer::~CreateCharacterRenderer() {
    if (field_font) {
        TTF_CloseFont(field_font);
        field_font = nullptr;
    }
    if (title_font) {
        TTF_CloseFont(title_font);
        title_font = nullptr;
    }
}

void CreateCharacterRenderer::init_layout() {
    const int logo_w = logo_texture.GetWidth();
    const int logo_h = logo_texture.GetHeight();
    const int logo_x = std::max(0, (ui_cfg.window_w - logo_w) / 2);
    const int logo_y = ui_cfg.login_logo_y;
    logo_rect = SDL2pp::Rect(logo_x, logo_y, logo_w, logo_h);

    int title_w = 0, title_h = 0;
    if (title_font)
        TTF_SizeUTF8(title_font, "Create Character", &title_w, &title_h);
    const int title_x = std::max(0, (ui_cfg.window_w - title_w) / 2);
    const int title_y = logo_y + logo_h + ui_cfg.login_title_spacing;
    title_rect = SDL2pp::Rect(title_x, title_y, title_w, title_h);

    const int field_x = std::max(0, (ui_cfg.window_w - ui_cfg.login_field_w) / 2);
    const int field_start_y = title_y + title_h + ui_cfg.login_field_spacing;
    username_field_rect =
            SDL2pp::Rect(field_x, field_start_y, ui_cfg.login_field_w, ui_cfg.login_field_h);
    const int password_y = field_start_y + ui_cfg.login_field_h + ui_cfg.login_field_gap;
    password_field_rect =
            SDL2pp::Rect(field_x, password_y, ui_cfg.login_field_w, ui_cfg.login_field_h);

    // Selector rows: each option = field_w/4 wide, same height as fields
    const int sel_w = ui_cfg.login_field_w / 4;
    const int sel_h = ui_cfg.login_field_h;
    const int sel_x = field_x;
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
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(background_texture, SDL2pp::NullOpt, background_rect);
    renderer.Copy(logo_texture, SDL2pp::NullOpt, logo_rect);

    SDL_Surface* title_surf = TTF_RenderUTF8_Blended(title_font, "Create Character", title_color);
    if (title_surf) {
        SDL2pp::Surface wrapped(title_surf);
        SDL2pp::Texture tex(renderer, wrapped);
        renderer.Copy(tex, SDL2pp::NullOpt, title_rect);
    }

    render_text_field(username_field_rect, username_model.get_text(), username_model.is_focused(),
                      ui_cfg.placeholder_username);
    render_text_field(password_field_rect, password_model.get_text(), password_model.is_focused(),
                      ui_cfg.placeholder_password);

    const int race_idx = static_cast<int>(selected_race) - 1;
    std::array<std::string, 4> race_labels = {"Human", "Elf", "Dwarf", "Gnome"};
    render_selector(race_rects, race_labels, race_idx);

    const int class_idx = static_cast<int>(selected_class) - 1;
    std::array<std::string, 4> class_labels = {"Mage", "Cleric", "Paladin", "Warrior"};
    render_selector(class_rects, class_labels, class_idx);

    create_button.render(renderer);
    back_button.render(renderer);
    render_error();
    renderer.Present();
}

void CreateCharacterRenderer::render_text_field(const SDL2pp::Rect& rect, const std::string& text,
                                                bool focused,
                                                const std::string& placeholder) const {
    form_widgets_->render_text_field(rect, text, focused, placeholder);
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

void CreateCharacterRenderer::render_error() const {
    const int below_y = create_button.rect.GetY() + create_button.rect.GetH();
    form_widgets_->render_error_centered(error_text, error_color, ui_cfg.window_w, below_y,
                                         ui_cfg.error_spacing);
}

bool CreateCharacterRenderer::is_username_hit(int x, int y) const {
    return point_in_rect(x, y, username_field_rect);
}
bool CreateCharacterRenderer::is_password_hit(int x, int y) const {
    return point_in_rect(x, y, password_field_rect);
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
void CreateCharacterRenderer::set_error(const std::string& text) { error_text = text; }
void CreateCharacterRenderer::clear_error() { error_text.clear(); }
