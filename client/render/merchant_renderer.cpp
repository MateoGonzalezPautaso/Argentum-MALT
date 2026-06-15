#include "merchant_renderer.h"

#include <string>

#include "text_renderer.h"
#include "texture_loader.h"

MerchantRenderer::MerchantRenderer(SDL2pp::Renderer& renderer, const UIConfig& cfg):
        bg_texture(renderer, texture::load_surface(cfg.asset_merchant_bg)),
        panel_rect(cfg.merchant.panel_x, cfg.merchant.panel_y, cfg.merchant.panel_w,
                   cfg.merchant.panel_h),
        buy_button(SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_buy_default)),
                   SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_buy_hover))),
        sell_button(SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_sell_default)),
                    SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_sell_hover))),
        plus_button(SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_plus_default)),
                    SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_plus_hover))),
        minus_button(SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_minus_default)),
                     SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_minus_hover))),
        gold_rect(cfg.merchant.gold_rect),
        font(TTF_OpenFont(cfg.font_path.c_str(), cfg.font_bar_size)) {
    const auto& m = cfg.merchant;
    buy_button.set_position(m.panel_x + m.buy.x,    m.panel_y + m.buy.y,    m.buy.w,   m.buy.h);
    sell_button.set_position(m.panel_x + m.sell.x,  m.panel_y + m.sell.y,   m.sell.w,  m.sell.h);
    plus_button.set_position(m.panel_x + m.plus.x,  m.panel_y + m.plus.y,   m.plus.w,  m.plus.h);
    minus_button.set_position(m.panel_x + m.minus.x, m.panel_y + m.minus.y, m.minus.w, m.minus.h);
}

MerchantRenderer::~MerchantRenderer() {
    if (font)
        TTF_CloseFont(font);
}

void MerchantRenderer::render(SDL2pp::Renderer& renderer, uint32_t gold) {
    renderer.Copy(bg_texture, SDL2pp::NullOpt, panel_rect);
    buy_button.render(renderer);
    sell_button.render(renderer);
    plus_button.render(renderer);
    minus_button.render(renderer);

    if (font) {
        auto result = texture::render_text(renderer, font, std::to_string(gold), gold_color);
        if (result.w > 0) {
            int tx = panel_rect.GetX() + gold_rect.x + (gold_rect.w - result.w) / 2;
            int ty = panel_rect.GetY() + gold_rect.y + (gold_rect.h - result.h) / 2;
            renderer.Copy(result.texture, SDL2pp::NullOpt,
                          SDL2pp::Rect(tx, ty, result.w, result.h));
        }
    }
}

void MerchantRenderer::set_buy_hovered(int x, int y) {
    buy_button.hovered = buy_button.is_hit(x, y);
}

void MerchantRenderer::set_sell_hovered(int x, int y) {
    sell_button.hovered = sell_button.is_hit(x, y);
}

void MerchantRenderer::set_plus_hovered(int x, int y) {
    plus_button.hovered = plus_button.is_hit(x, y);
}

void MerchantRenderer::set_minus_hovered(int x, int y) {
    minus_button.hovered = minus_button.is_hit(x, y);
}

bool MerchantRenderer::is_any_button_hovered() const {
    return buy_button.hovered || sell_button.hovered || plus_button.hovered ||
           minus_button.hovered;
}
