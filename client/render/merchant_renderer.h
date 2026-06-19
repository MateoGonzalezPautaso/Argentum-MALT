#ifndef CLIENT_MERCHANT_RENDERER_H
#define CLIENT_MERCHANT_RENDERER_H

#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../common/messages.h"
#include "../config/config.h"
#include "button.h"

class MerchantRenderer {
public:
    explicit MerchantRenderer(SDL2pp::Renderer& renderer, const UIConfig& cfg);
    ~MerchantRenderer();

    void render(SDL2pp::Renderer& renderer);

    void set_buy_hovered(int x, int y);
    void set_sell_hovered(int x, int y);

    bool is_any_button_hovered() const;

    void set_items(const std::vector<NpcItemEntry>& items);
    void set_selected(int idx);
    void set_scroll(int offset);
    int max_scroll() const;
    int item_at(int x, int y) const;
    bool is_buy_hit(int x, int y) const;
    bool is_sell_hit(int x, int y) const;
    SDL2pp::Rect panel_bounds() const { return panel_rect; }

private:
    SDL2pp::Texture bg_texture;
    SDL2pp::Rect panel_rect;
    Button buy_button;
    Button sell_button;
    TTF_Font* font = nullptr;

    std::vector<NpcItemEntry> items_;
    int selected_idx_ = -1;
    int scroll_offset_ = 0;
    int list_x_ = 0;
    int list_y_ = 0;
    int list_w_ = 0;
    int list_h_ = 0;
    int row_h_ = 0;
};

#endif
