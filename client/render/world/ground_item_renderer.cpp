#include "ground_item_renderer.h"

#include <algorithm>
#include <cmath>

#include <SDL2/SDL.h>

#include "../texture_loader.h"

namespace {
constexpr int PHASE_HASH_X_PRIME = 137;
constexpr int PHASE_HASH_Y_PRIME = 31;
constexpr int PHASE_HASH_T_PRIME = 53;
constexpr uint8_t FALLBACK_COLOR = 80;
constexpr uint8_t FALLBACK_ALPHA = 200;
}  // namespace

GroundItemRenderer::GroundItemRenderer(
        SDL2pp::Renderer& renderer,
        const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites,
        const GroundItemConfig& cfg):
        renderer(renderer), item_sprites(item_sprites), config_(cfg) {}

void GroundItemRenderer::add_item(int world_x, int world_y, ItemType type,
                                  const std::string& name) {
    items[{world_x, world_y}].push_back(GroundItem{type, name});
}

void GroundItemRenderer::remove_item(int world_x, int world_y, const std::string& item_name) {
    auto it = items.find({world_x, world_y});
    if (it == items.end())
        return;
    auto& vec = it->second;
    auto elem = std::find_if(vec.begin(), vec.end(),
                             [&](const GroundItem& g) { return g.name == item_name; });
    if (elem != vec.end())
        vec.erase(elem);
    if (vec.empty())
        items.erase(it);
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
    SDL2pp::Rect dst(screen_x, screen_y, config_.display_size, config_.display_size);

    auto it = item_sprites.find(static_cast<uint8_t>(item.type));
    if (it != item_sprites.end() && !it->second.path.empty()) {
        const ItemSpriteDef& def = it->second;
        SDL2pp::Texture* tex = get_or_load_texture(def);
        SDL2pp::Rect src(def.src_x, def.src_y, def.src_w, def.src_h);
        renderer.Copy(*tex, src, dst);
    } else {
        uint8_t r = FALLBACK_COLOR, g = FALLBACK_COLOR, b = FALLBACK_COLOR;
        if (it != item_sprites.end()) {
            r = it->second.color_r;
            g = it->second.color_g;
            b = it->second.color_b;
        }
        renderer.SetDrawColor(r, g, b, FALLBACK_ALPHA);
        renderer.FillRect(dst);
        renderer.SetDrawColor(255, 255, 255, FALLBACK_ALPHA);
        renderer.DrawRect(dst);
    }
}

void GroundItemRenderer::render(const SDL2pp::Rect& cam) {
    const int vw = cam.GetW();
    const int vh = cam.GetH();
    const int half = config_.display_size / 2;

    uint32_t ticks = SDL_GetTicks();

    for (const auto& [pos, vec]: items) {
        int screen_x = pos.first - cam.GetX() - half;
        int screen_y = pos.second - cam.GetY() - half;

        if (screen_x + config_.display_size < 0 || screen_x > vw)
            continue;
        if (screen_y + config_.display_size < 0 || screen_y > vh)
            continue;

        for (const auto& item: vec) {
            int phase = (pos.first * PHASE_HASH_X_PRIME + pos.second * PHASE_HASH_Y_PRIME +
                         static_cast<int>(item.type) * PHASE_HASH_T_PRIME) %
                        static_cast<int>(config_.float_period_ms);
            double t = static_cast<double>((ticks + phase) % config_.float_period_ms) /
                       config_.float_period_ms;
            int float_offset = static_cast<int>(
                    std::round(config_.float_amplitude * std::sin(2.0 * M_PI * t)));

            draw_item(item, screen_x, screen_y + float_offset);
        }
    }
}
