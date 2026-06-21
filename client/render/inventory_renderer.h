#ifndef CLIENT_INVENTORY_RENDERER_H
#define CLIENT_INVENTORY_RENDERER_H

#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../common/messages.h"
#include "../config/config.h"
#include "item_sprite_drawer.h"
#include "texture_cache.h"

class InventoryRenderer {
private:
    SDL2pp::Renderer& renderer;
    TTF_Font* font;
    const InventoryPanelConfig& cfg;
    const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites;
    TextureCache texture_cache_;
    ItemSpriteDrawer item_drawer_;

    int hovered_inv_slot = -1;
    int hovered_equip_slot = -1;

    const ItemSpriteDef* find_sprite(ItemType type) const;

    void draw_item_sprite(ItemType type, const std::string& name, const SDL2pp::Rect& dst);
    void draw_item_tooltip(int sx, int sy, const InventorySlot& slot);
    void draw_hover_border(int sx, int sy);

public:
    InventoryRenderer(SDL2pp::Renderer& renderer, TTF_Font* font, const InventoryPanelConfig& cfg,
                      const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites);

    void set_font(TTF_Font* f) { font = f; }

    void update_hover(int mx, int my, const std::vector<InventorySlot>& slots,
                      const InventorySlot equipped[EQUIP_SLOT_COUNT]);
    bool is_hovering_occupied() const;
    int get_hovered_inv_slot() const { return hovered_inv_slot; }
    int get_hovered_equip_slot() const { return hovered_equip_slot; }

    void render(const std::vector<InventorySlot>& slots);
    void render_equipped(const InventorySlot equipped[EQUIP_SLOT_COUNT]);
};

#endif  // CLIENT_INVENTORY_RENDERER_H
