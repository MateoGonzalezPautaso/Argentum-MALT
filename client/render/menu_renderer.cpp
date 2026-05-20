#include "menu_renderer.h"

#include <algorithm>

#include "texture_loader.h"

MenuRenderer::MenuRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h):
        renderer(renderer),
        menu_background_texture(renderer,
                                load_surface("assets/interface/en_ventanalauncher.bmp")),
        start_button(SDL2pp::Texture(renderer,
                                     load_surface("assets/interface/en_boton-comenzar-default.bmp")),
                     SDL2pp::Texture(renderer,
                                     load_surface("assets/interface/en_boton-comenzar-over.bmp"))),
        settings_button(
                SDL2pp::Texture(renderer,
                                load_surface("assets/interface/en_boton-config-default.bmp")),
                SDL2pp::Texture(renderer,
                                load_surface("assets/interface/en_boton-config-over.bmp"))),
        menu_background_rect(0, 0, window_w, window_h),
        window_w(window_w),
        window_h(window_h) {
    init_layout();
}

void MenuRenderer::render() {
    renderer.Copy(menu_background_texture, SDL2pp::NullOpt, menu_background_rect);
    settings_button.render(renderer);
    start_button.render(renderer);
}

bool MenuRenderer::is_start_hit(int x, int y) const { return start_button.is_hit(x, y); }

bool MenuRenderer::is_settings_hit(int x, int y) const { return settings_button.is_hit(x, y); }

void MenuRenderer::set_start_button_hovered(int x, int y) {
    start_button.hovered = start_button.is_hit(x, y);
}

void MenuRenderer::set_settings_button_hovered(int x, int y) {
    settings_button.hovered = settings_button.is_hit(x, y);
}

void MenuRenderer::init_layout() {
    // START button
    const int start_w = 330;
    const int start_h = 70;
    const int start_x = (window_w - start_w) / 2;  // 352
    start_button.set_position(start_x, 565, start_w, start_h);

    // SETTINGS button
    const int settings_w = 270;
    const int settings_h = 45;
    settings_button.set_position(688, 335, settings_w, settings_h);
}
