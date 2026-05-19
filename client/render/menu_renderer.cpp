#include "menu_renderer.h"

#include <algorithm>

#include "texture_loader.h"

MenuRenderer::MenuRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h):
        renderer(renderer),
        menu_background_texture(renderer,
                                load_surface("assets/BabelUI/static/media/leather_black..png")),
        menu_logo_texture(renderer, load_surface("assets/BabelUI/static/media/ao20_logo_med..png")),
        start_button(SDL2pp::Texture(renderer,
                                     load_surface("assets/interface/en_boton-comenzar-default.bmp")),
                     SDL2pp::Texture(renderer,
                                     load_surface("assets/interface/en_boton-comenzar-over.bmp"))),
        menu_background_rect(0, 0, window_w, window_h),
        menu_logo_rect(0, 0, 0, 0),
        window_w(window_w),
        window_h(window_h) {
    init_layout();
}

void MenuRenderer::render() {
    renderer.Copy(menu_background_texture, SDL2pp::NullOpt, menu_background_rect);
    renderer.Copy(menu_logo_texture, SDL2pp::NullOpt, menu_logo_rect);
    start_button.render(renderer);
}

bool MenuRenderer::is_button_hit(int x, int y) const { return start_button.is_hit(x, y); }

void MenuRenderer::set_button_hovered(int x, int y) {
    start_button.hovered = start_button.is_hit(x, y);
}

void MenuRenderer::init_layout() {
    const int logo_w = menu_logo_texture.GetWidth();
    const int logo_h = menu_logo_texture.GetHeight();
    const int button_w = start_button.default_tex.GetWidth();
    const int button_h = start_button.default_tex.GetHeight();

    const int logo_x = std::max(0, (window_w - logo_w) / 2);
    const int logo_y = std::max(0, (window_h - logo_h) / 2 - 40);
    const int button_x = std::max(0, (window_w - button_w) / 2);
    const int button_y = std::min(window_h - button_h, logo_y + logo_h + 20);
    menu_logo_rect = SDL2pp::Rect(logo_x, logo_y, logo_w, logo_h);
    start_button.set_position(button_x, button_y, button_w, button_h);
}
