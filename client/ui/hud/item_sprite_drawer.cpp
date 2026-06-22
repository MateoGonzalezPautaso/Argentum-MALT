#include "item_sprite_drawer.h"

#include "../../render/gfx/text_renderer.h"

ItemSpriteDrawer::ItemSpriteDrawer(TextureCache& cache): cache_(cache) {}

void ItemSpriteDrawer::draw(SDL2pp::Renderer& renderer, const ItemSpriteDef* def,
                            const SDL2pp::Rect& dst, uint8_t fallback_alpha, TTF_Font* font,
                            const std::string& name) const {
    if (def && !def->path.empty()) {
        SDL2pp::Texture& tex = cache_.get(def->path);
        renderer.Copy(tex, SDL2pp::Rect(def->src_x, def->src_y, def->src_w, def->src_h), dst);
        return;
    }

    uint8_t r = 80, g = 80, b = 80;
    if (def) {
        r = def->color_r;
        g = def->color_g;
        b = def->color_b;
    }
    renderer.SetDrawColor(r, g, b, fallback_alpha);
    renderer.FillRect(dst);
    renderer.SetDrawColor(255, 255, 255, fallback_alpha);
    renderer.DrawRect(dst);

    if (font && !name.empty()) {
        auto result = texture::render_text(renderer, font, name, {255, 255, 255, 255});
        if (result.w > 0) {
            const int tx = dst.GetX() + (dst.GetW() - result.w) / 2;
            const int ty = dst.GetY() + (dst.GetH() - result.h) / 2;
            renderer.Copy(result.texture, SDL2pp::NullOpt,
                          SDL2pp::Rect(tx, ty, result.w, result.h));
        }
    }
}
