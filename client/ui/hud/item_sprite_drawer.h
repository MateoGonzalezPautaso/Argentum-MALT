#ifndef CLIENT_RENDER_ITEM_SPRITE_DRAWER_H
#define CLIENT_RENDER_ITEM_SPRITE_DRAWER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../config/config.h"
#include "../../render/gfx/texture_cache.h"


class ItemSpriteDrawer {
public:
    explicit ItemSpriteDrawer(TextureCache& cache);

    // Renders `def` into `dst`. If def is null or has no texture path, fills with the
    // def's fallback color (or grey if def is null). Optionally renders `name` centered
    // in the fallback rect when `font` is not null.
    void draw(SDL2pp::Renderer& renderer, const ItemSpriteDef* def, const SDL2pp::Rect& dst,
              uint8_t fallback_alpha = 255, TTF_Font* font = nullptr,
              const std::string& name = {}) const;

private:
    TextureCache& cache_;
};

#endif  // CLIENT_RENDER_ITEM_SPRITE_DRAWER_H
