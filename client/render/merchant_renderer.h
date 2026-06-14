#ifndef CLIENT_MERCHANT_RENDERER_H
#define CLIENT_MERCHANT_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>

class MerchantRenderer {
public:
    MerchantRenderer(SDL2pp::Renderer& renderer, const std::string& asset_path, int x, int y,
                     int w, int h);

    void render(SDL2pp::Renderer& renderer);

private:
    SDL2pp::Texture bg_texture;
    SDL2pp::Rect panel_rect;
};

#endif
