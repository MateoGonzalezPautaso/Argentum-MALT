#include "menu_renderer.h"

#include <algorithm>

#include "texture_loader.h"

MenuRenderer::MenuRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h):
        renderer(renderer),
        menu_background_texture(renderer,
                                load_surface("assets/BabelUI/static/media/leather_black..png")),
        menu_logo_texture(renderer, load_surface("assets/BabelUI/static/media/ao20_logo_med..png")),
        menu_button_texture(renderer,
                            load_surface("assets/interface/en_boton-comenzar-default.bmp")),
        menu_background_rect(0, 0, window_w, window_h),
        menu_logo_rect(0, 0, 0, 0),
        menu_button_rect(0, 0, 0, 0),
        window_w(window_w),
        window_h(window_h) {
    init_layout();
}

void MenuRenderer::render() {
    renderer.Copy(menu_background_texture, SDL2pp::NullOpt, menu_background_rect);
    renderer.Copy(menu_logo_texture, SDL2pp::NullOpt, menu_logo_rect);
    renderer.Copy(menu_button_texture, SDL2pp::NullOpt, menu_button_rect);
}

bool MenuRenderer::is_button_hit(int x, int y) const {
    // hitbox rectangular para boton start
    const int left = menu_button_rect.GetX();
    const int top = menu_button_rect.GetY();
    const int right = left + menu_button_rect.GetW();
    const int bottom = top + menu_button_rect.GetH();
    return x >= left && x <= right && y >= top && y <= bottom;
}

void MenuRenderer::init_layout() {
    // centra logo y boton verticalmente
    const int logo_w = menu_logo_texture.GetWidth();
    const int logo_h = menu_logo_texture.GetHeight();
    const int button_w = menu_button_texture.GetWidth();
    const int button_h = menu_button_texture.GetHeight();

    const int logo_x = std::max(0, (window_w - logo_w) / 2);
    const int logo_y = std::max(0, (window_h - logo_h) / 2 - 40);
    const int button_x = std::max(0, (window_w - button_w) / 2);
    const int button_y = std::min(window_h - button_h, logo_y + logo_h + 20);
    menu_logo_rect = SDL2pp::Rect(logo_x, logo_y, logo_w, logo_h);
    menu_button_rect = SDL2pp::Rect(button_x, button_y, button_w, button_h);
}
