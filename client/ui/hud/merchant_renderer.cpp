#include "merchant_renderer.h"

#include <stdexcept>
#include <string>

#include "../../render/gfx/text_renderer.h"
#include "../../render/gfx/texture_loader.h"

MerchantRenderer::MerchantRenderer(SDL2pp::Renderer& renderer, const UIConfig& cfg):
        bg_texture(renderer, texture::load_surface(cfg.asset_merchant_bg)),
        panel_rect(cfg.merchant.panel_x, cfg.merchant.panel_y, cfg.merchant.panel_w,
                   cfg.merchant.panel_h),
        buy_button(SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_buy_default)),
                   SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_buy_hover))),
        sell_button(SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_sell_default)),
                    SDL2pp::Texture(renderer, texture::load_surface(cfg.asset_sell_hover))),
        sell_disabled_tex_(renderer, texture::load_surface(cfg.asset_sell_disabled)),
        font(TTF_OpenFont(cfg.font_path.c_str(), cfg.font_bar_size)),
        list_x_(cfg.merchant.panel_x + cfg.merchant.list_offset_x),
        list_y_(cfg.merchant.panel_y + cfg.merchant.list_offset_y),
        list_w_(cfg.merchant.list_w),
        list_h_(cfg.merchant.list_h),
        row_h_(cfg.merchant.row_h),
        price_x_(cfg.merchant.panel_x + cfg.merchant.price_offset_x),
        sell_price_x_(cfg.merchant.panel_x + cfg.merchant.sell_price_offset_x),
        sell_price_ratio_(cfg.merchant.sell_price_ratio) {
    if (!font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }

    const auto& m = cfg.merchant;
    buy_button.set_position(m.panel_x + m.buy.x,   m.panel_y + m.buy.y,  m.buy.w,  m.buy.h);
    sell_button.set_position(m.panel_x + m.sell.x, m.panel_y + m.sell.y, m.sell.w, m.sell.h);
}

MerchantRenderer::~MerchantRenderer() {
    if (font)
        TTF_CloseFont(font);
}

void MerchantRenderer::render(SDL2pp::Renderer& renderer) {
    renderer.Copy(bg_texture, SDL2pp::NullOpt, panel_rect);
    buy_button.render(renderer);
    if (sell_enabled_) {
        sell_button.render(renderer);
    } else {
        renderer.Copy(sell_disabled_tex_, SDL2pp::NullOpt, sell_button.rect);
    }

    if (!font || items_.empty())
        return;

    SDL_Color name_color{220, 220, 220, 255};
    SDL_Color price_color{255, 215, 0, 255};

    for (int i = scroll_offset_; i < static_cast<int>(items_.size()); ++i) {
        int ry = list_y_ + (i - scroll_offset_) * row_h_;
        if (ry + row_h_ > list_y_ + list_h_)
            break;

        SDL2pp::Rect row_rect(list_x_, ry, list_w_, row_h_);

        if (i == selected_idx_) {
            renderer.SetDrawColor(200, 160, 40, 120);
            renderer.FillRect(row_rect);
        }

        texture::render_text_clipped(renderer, font, items_[i].item_name, name_color,
                                     SDL2pp::Rect(list_x_ + 4, ry, list_w_ - 90, row_h_),
                                     0, 0, true);
        texture::render_text_clipped(renderer, font,
                                     std::to_string(items_[i].price) + "g", price_color,
                                     SDL2pp::Rect(price_x_, ry, sell_price_x_ - price_x_, row_h_),
                                     0, 0, true);

        uint32_t sell_price = static_cast<uint32_t>(items_[i].price * sell_price_ratio_);
        texture::render_text_clipped(renderer, font,
                                     std::to_string(sell_price) + "g", price_color,
                                     SDL2pp::Rect(sell_price_x_, ry, list_x_ + list_w_ - sell_price_x_, row_h_),
                                     0, 0, true);
    }
}

void MerchantRenderer::set_items(const std::vector<NpcItemEntry>& items) {
    items_ = items;
}

void MerchantRenderer::set_selected(int idx) {
    selected_idx_ = idx;
}

int MerchantRenderer::item_at(int x, int y) const {
    if (items_.empty() || x < list_x_ || x >= list_x_ + list_w_ || y < list_y_)
        return -1;
    if (y >= list_y_ + list_h_)
        return -1;
    int idx = scroll_offset_ + (y - list_y_) / row_h_;
    if (idx < 0 || idx >= static_cast<int>(items_.size()))
        return -1;
    return idx;
}

void MerchantRenderer::set_scroll(int offset) {
    scroll_offset_ = std::max(0, std::min(offset, max_scroll()));
}

int MerchantRenderer::max_scroll() const {
    int visible_rows = list_h_ / row_h_;
    return std::max(0, static_cast<int>(items_.size()) - visible_rows);
}

bool MerchantRenderer::is_buy_hit(int x, int y) const {
    return buy_button.is_hit(x, y);
}

bool MerchantRenderer::is_sell_hit(int x, int y) const {
    return sell_enabled_ && sell_button.is_hit(x, y);
}

void MerchantRenderer::set_sell_enabled(bool enabled) {
    sell_enabled_ = enabled;
}

void MerchantRenderer::set_buy_hovered(int x, int y) {
    buy_button.hovered = buy_button.is_hit(x, y);
}

void MerchantRenderer::set_sell_hovered(int x, int y) {
    sell_button.hovered = sell_button.is_hit(x, y);
}

bool MerchantRenderer::is_any_button_hovered() const {
    return buy_button.hovered || sell_button.hovered;
}
