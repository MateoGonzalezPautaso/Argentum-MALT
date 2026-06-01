#ifndef CLIENT_INVENTORY_RENDERER_H
#define CLIENT_INVENTORY_RENDERER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../common/messages.h"
#include "../config/config.h"

class InventoryRenderer {
private:
    SDL2pp::Renderer& renderer;
    TTF_Font* font;
    const InventoryPanelConfig& cfg;
    const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites;
    std::unordered_map<std::string, std::unique_ptr<SDL2pp::Texture>> texture_cache;

    SDL_Color color_for_type(ItemType type) const;
    const ItemSpriteDef* find_sprite(ItemType type) const;

public:
    InventoryRenderer(SDL2pp::Renderer& renderer, TTF_Font* font,
                      const InventoryPanelConfig& cfg,
                      const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites);

    void set_font(TTF_Font* f) { font = f; }

    void render(const std::vector<InventorySlot>& slots);
    void render_equipped(const InventorySlot equipped[4]);
};

#endif  // CLIENT_INVENTORY_RENDERER_H
