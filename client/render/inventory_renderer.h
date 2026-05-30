#ifndef CLIENT_INVENTORY_RENDERER_H
#define CLIENT_INVENTORY_RENDERER_H

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

    SDL_Color color_for_type(ItemType type) const;

public:
    InventoryRenderer(SDL2pp::Renderer& renderer, TTF_Font* font,
                      const InventoryPanelConfig& cfg);

    void set_font(TTF_Font* f) { font = f; }

    void render(const std::vector<InventorySlot>& slots);
};

#endif  // CLIENT_INVENTORY_RENDERER_H
