#ifndef CLIENT_MERCHANT_RENDERER_H
#define CLIENT_MERCHANT_RENDERER_H

#include <SDL2pp/SDL2pp.hh>

#include "../config/config.h"
#include "button.h"

class MerchantRenderer {
public:
    explicit MerchantRenderer(SDL2pp::Renderer& renderer, const UIConfig& cfg);

    void render(SDL2pp::Renderer& renderer);

    void set_buy_hovered(int x, int y);
    void set_sell_hovered(int x, int y);
    void set_plus_hovered(int x, int y);
    void set_minus_hovered(int x, int y);

    bool is_any_button_hovered() const;

private:
    SDL2pp::Texture bg_texture;
    SDL2pp::Rect panel_rect;
    Button buy_button;
    Button sell_button;
    Button plus_button;
    Button minus_button;
};

#endif
