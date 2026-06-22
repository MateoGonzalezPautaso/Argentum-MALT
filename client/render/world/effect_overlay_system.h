#ifndef CLIENT_EFFECT_OVERLAY_SYSTEM_H
#define CLIENT_EFFECT_OVERLAY_SYSTEM_H

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include <SDL2pp/SDL2pp.hh>

#include "../../config/config.h"

struct OverlayEffect {
    std::vector<SDL2pp::Texture> frames;
    SDL2pp::Rect dst;
    bool animated = true;
    uint32_t frame_ms = 70;
    std::size_t current_frame = 0;
    uint32_t last_ticks = 0;
    bool active = false;
};

class EffectOverlaySystem {
public:
    explicit EffectOverlaySystem(SDL2pp::Renderer& renderer);

    void load_damage_overlay(const DamageOverlayConfig& cfg);
    void trigger_damage_effect(int world_x, int world_y);
    void load_spell_sheets(const std::vector<SpellSheetConfig>& sheets);
    void trigger_spell_effect(uint8_t effect_type, int world_x, int world_y);

    void advance();
    void render(const SDL2pp::Rect& cam);

private:
    SDL2pp::Renderer& renderer;
    std::vector<OverlayEffect> overlays;
    std::unordered_map<uint8_t, std::vector<OverlayEffect>> spell_pools;
    std::unordered_map<uint8_t, std::pair<int, int>> spell_offsets;
};

#endif  // CLIENT_EFFECT_OVERLAY_SYSTEM_H
