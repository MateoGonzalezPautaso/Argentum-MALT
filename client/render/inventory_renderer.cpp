#include "inventory_renderer.h"

#include "text_renderer.h"

InventoryRenderer::InventoryRenderer(SDL2pp::Renderer& renderer, TTF_Font* font,
                                     const InventoryPanelConfig& cfg):
        renderer(renderer), font(font), cfg(cfg) {}

SDL_Color InventoryRenderer::color_for_type(ItemType type) const {
    switch (type) {
        case ItemType::SWORD:
        case ItemType::AXE:
        case ItemType::HAMMER:
            return {180, 40, 40, 255};
        case ItemType::ASH_STAFF:
        case ItemType::ELVEN_FLUTE:
        case ItemType::KNOTTED_STAFF:
        case ItemType::STUDDED_STAFF:
            return {140, 40, 200, 255};
        case ItemType::SIMPLE_BOW:
        case ItemType::COMPOSITE_BOW:
            return {200, 120, 40, 255};
        case ItemType::LEATHER_ARMOR:
        case ItemType::PLATE_ARMOR:
        case ItemType::BLUE_TUNIC:
            return {40, 80, 180, 255};
        case ItemType::HOOD:
        case ItemType::IRON_HELMET:
        case ItemType::MAGIC_HAT:
            return {180, 160, 40, 255};
        case ItemType::TURTLE_SHIELD:
        case ItemType::IRON_SHIELD:
            return {40, 160, 180, 255};
        case ItemType::HEALTH_POTION:
        case ItemType::MANA_POTION:
            return {40, 140, 60, 255};
        default:
            return {80, 80, 80, 255};
    }
}

void InventoryRenderer::render(const std::vector<InventorySlot>& slots) {
    if (!font) return;

    for (size_t i = 0; i < slots.size(); ++i) {
        int col = static_cast<int>(i) % cfg.cols;
        int row = static_cast<int>(i) / cfg.cols;
        int sx = cfg.x + col * (cfg.slot_w + cfg.gap);
        int sy = cfg.y + row * (cfg.slot_h + cfg.gap);
        SDL2pp::Rect slot_rect(sx, sy, cfg.slot_w, cfg.slot_h);

        if (slots[i].item_type == ItemType::NONE) {
            renderer.SetDrawColor(40, 40, 40, 255);
            renderer.FillRect(slot_rect);
            renderer.SetDrawColor(80, 80, 80, 255);
            renderer.DrawRect(slot_rect);
        } else {
            SDL_Color bg = color_for_type(slots[i].item_type);
            renderer.SetDrawColor(bg.r, bg.g, bg.b, 255);
            renderer.FillRect(slot_rect);
            renderer.SetDrawColor(255, 255, 255, 255);
            renderer.DrawRect(slot_rect);

            auto result = texture::render_text(renderer, font, slots[i].item_name,
                                                {255, 255, 255, 255});
            if (result.w > 0) {
                int tx = sx + (cfg.slot_w - result.w) / 2;
                int ty = sy + (cfg.slot_h - result.h) / 2;
                SDL2pp::Rect text_dst(tx, ty, result.w, result.h);
                renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
            }
        }
    }
}
