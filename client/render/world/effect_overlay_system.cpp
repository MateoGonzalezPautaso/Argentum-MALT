#include "effect_overlay_system.h"

#include <SDL2/SDL.h>

#include "../gfx/texture_loader.h"

EffectOverlaySystem::EffectOverlaySystem(SDL2pp::Renderer& renderer): renderer(renderer) {}

void EffectOverlaySystem::load_damage_overlay(const DamageOverlayConfig& cfg) {
    overlays.resize(3);
    for (auto& ov: overlays) {
        ov.frames.reserve(cfg.last_graphic - cfg.first_graphic + 1);
        for (int i = cfg.first_graphic; i <= cfg.last_graphic; ++i) {
            std::string path = "assets/Graficos/" + std::to_string(i) + ".png";
            ov.frames.emplace_back(renderer, texture::load_surface(path));
        }
        ov.frame_ms = cfg.frame_ms;
        ov.dst.SetW(cfg.display_w);
        ov.dst.SetH(cfg.display_h);
    }
}

void EffectOverlaySystem::load_spell_sheets(const std::vector<SpellSheetConfig>& sheets) {
    for (uint8_t idx = 0; idx < static_cast<uint8_t>(sheets.size()); ++idx) {
        const SpellSheetConfig& s = sheets[idx];
        spell_offsets[idx] = {s.offset_x, s.offset_y};
        SDL2pp::Surface sheet = texture::load_surface(s.path);
        std::vector<OverlayEffect>& pool = spell_pools[idx];
        pool.resize(3);
        const int total = s.cols * s.rows;
        for (auto& ov: pool) {
            ov.frames.reserve(total);
            for (int r = 0; r < s.rows; ++r) {
                for (int c = 0; c < s.cols; ++c) {
                    SDL_Surface* sub = SDL_CreateRGBSurfaceWithFormat(0, s.frame_w, s.frame_h, 32,
                                                                      SDL_PIXELFORMAT_ABGR8888);
                    SDL_Rect src{c * s.frame_w, r * s.frame_h, s.frame_w, s.frame_h};
                    SDL_SetSurfaceBlendMode(sheet.Get(), SDL_BLENDMODE_NONE);
                    SDL_BlitSurface(sheet.Get(), &src, sub, nullptr);
                    ov.frames.emplace_back(renderer, SDL2pp::Surface(sub))
                            .SetBlendMode(SDL_BLENDMODE_BLEND);
                }
            }
            ov.frame_ms = s.frame_ms;
            ov.dst.SetW(s.display_w);
            ov.dst.SetH(s.display_h);
        }
    }
}

void EffectOverlaySystem::trigger_damage_effect(int world_x, int world_y) {
    for (auto& ov: overlays) {
        if (ov.active)
            continue;
        ov.dst.SetX(world_x - ov.dst.GetW() / 2);
        ov.dst.SetY(world_y - ov.dst.GetH() / 2);
        ov.current_frame = 0;
        ov.last_ticks = SDL_GetTicks();
        ov.active = true;
        return;
    }
}

void EffectOverlaySystem::trigger_spell_effect(uint8_t effect_type, int world_x, int world_y) {
    auto it = spell_pools.find(effect_type);
    if (it == spell_pools.end())
        return;
    for (auto& ov: it->second) {
        if (ov.active)
            continue;
        int ox = 0, oy = 0;
        auto off_it = spell_offsets.find(effect_type);
        if (off_it != spell_offsets.end()) {
            ox = off_it->second.first;
            oy = off_it->second.second;
        }
        ov.dst.SetX(world_x - ov.dst.GetW() / 2 + ox);
        ov.dst.SetY(world_y - ov.dst.GetH() / 2 + oy);
        ov.current_frame = 0;
        ov.last_ticks = SDL_GetTicks();
        ov.active = true;
        return;
    }
}

void EffectOverlaySystem::advance() {
    const uint32_t now = SDL_GetTicks();
    auto tick_pool = [&](std::vector<OverlayEffect>& pool) {
        for (auto& ov: pool) {
            if (!ov.active || now - ov.last_ticks < ov.frame_ms)
                continue;
            ov.last_ticks = now;
            if (++ov.current_frame >= ov.frames.size())
                ov.active = false;
        }
    };
    tick_pool(overlays);
    for (auto& [key, pool]: spell_pools) tick_pool(pool);
}

void EffectOverlaySystem::render(const SDL2pp::Rect& cam) {
    auto render_pool = [&](std::vector<OverlayEffect>& pool) {
        for (auto& ov: pool) {
            if (!ov.active)
                continue;
            SDL2pp::Rect dst(ov.dst.GetX() - cam.GetX(), ov.dst.GetY() - cam.GetY(), ov.dst.GetW(),
                             ov.dst.GetH());
            renderer.Copy(ov.frames[ov.current_frame], SDL2pp::NullOpt, dst);
        }
    };
    render_pool(overlays);
    for (auto& [key, pool]: spell_pools) render_pool(pool);
}
