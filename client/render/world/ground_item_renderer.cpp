#include "ground_item_renderer.h"

#include <cmath>

#include <SDL2/SDL.h>

#include "../texture_loader.h"

GroundItemRenderer::GroundItemRenderer(
        SDL2pp::Renderer& renderer,
        const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites,
        int display_size):
        renderer(renderer), item_sprites(item_sprites), display_size(display_size) {}

void GroundItemRenderer::add_item(int world_x, int world_y, ItemType type,
                                  const std::string& name) {
    items[{world_x, world_y}] = GroundItem{type, name};
}

void GroundItemRenderer::remove_item(int world_x, int world_y) {
    items.erase({world_x, world_y});
}

void GroundItemRenderer::clear() { items.clear(); }

SDL2pp::Texture* GroundItemRenderer::get_or_load_texture(const ItemSpriteDef& def) {
    auto it = texture_cache.find(def.path);
    if (it == texture_cache.end()) {
        SDL2pp::Surface surf = texture::load_surface(def.path);
        auto tex = std::make_unique<SDL2pp::Texture>(renderer, surf);
        it = texture_cache.emplace(def.path, std::move(tex)).first;
    }
    return it->second.get();
}

void GroundItemRenderer::draw_item(const GroundItem& item, int screen_x, int screen_y) {
    SDL2pp::Rect dst(screen_x, screen_y, display_size, display_size);

    auto it = item_sprites.find(static_cast<uint8_t>(item.type));
    if (it != item_sprites.end() && !it->second.path.empty()) {
        const ItemSpriteDef& def = it->second;
        SDL2pp::Texture* tex = get_or_load_texture(def);
        SDL2pp::Rect src(def.src_x, def.src_y, def.src_w, def.src_h);
        renderer.Copy(*tex, src, dst);
    } else {
        uint8_t r = 80, g = 80, b = 80;
        if (it != item_sprites.end()) {
            r = it->second.color_r;
            g = it->second.color_g;
            b = it->second.color_b;
        }
        renderer.SetDrawColor(r, g, b, 200);
        renderer.FillRect(dst);
        renderer.SetDrawColor(255, 255, 255, 200);
        renderer.DrawRect(dst);
    }
}

void GroundItemRenderer::render(const SDL2pp::Rect& cam) {
    const int vw = cam.GetW();
    const int vh = cam.GetH();

    uint32_t ticks = SDL_GetTicks();

    for (const auto& [pos, item]: items) {
        int screen_x = pos.first - cam.GetX() - display_size / 2;
        int screen_y = pos.second - cam.GetY() - display_size / 2;

        if (screen_x + display_size < 0 || screen_x > vw)
            continue;
        if (screen_y + display_size < 0 || screen_y > vh)
            continue;

        int phase = (pos.first * 137 + pos.second * 31) % static_cast<int>(float_period_ms_);
        double t = static_cast<double>((ticks + phase) % float_period_ms_) / float_period_ms_;
        int float_offset = static_cast<int>(std::round(float_amplitude_ * std::sin(2.0 * M_PI * t)));

        draw_item(item, screen_x, screen_y + float_offset);
    }
}
