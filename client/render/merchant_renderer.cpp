#include "merchant_renderer.h"

#include "texture_loader.h"

MerchantRenderer::MerchantRenderer(SDL2pp::Renderer& renderer, const std::string& asset_path, int x,
                                   int y, int w, int h):
        bg_texture(renderer, texture::load_surface(asset_path)), panel_rect(x, y, w, h) {}

void MerchantRenderer::render(SDL2pp::Renderer& renderer) {
    renderer.Copy(bg_texture, SDL2pp::NullOpt, panel_rect);
}
