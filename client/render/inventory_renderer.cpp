#include "inventory_renderer.h"

#include "geometry.h"
#include "text_renderer.h"

InventoryRenderer::InventoryRenderer(
        SDL2pp::Renderer& renderer, TTF_Font* font, const InventoryPanelConfig& cfg,
        const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites):
        renderer(renderer), font(font), cfg(cfg), item_sprites(item_sprites),
        texture_cache_(renderer), item_drawer_(texture_cache_) {}

const ItemSpriteDef* InventoryRenderer::find_sprite(ItemType type) const {
    auto it = item_sprites.find(static_cast<uint8_t>(type));
    return it != item_sprites.end() ? &it->second : nullptr;
}

void InventoryRenderer::render_item_sprite(ItemType type, const std::string& name,
                                           const SDL2pp::Rect& dst) {
    item_drawer_.draw(renderer, find_sprite(type), dst, 255, font, name);
}

void InventoryRenderer::update_hover(int mx, int my, const std::vector<InventorySlot>& slots,
                                     const InventorySlot equipped[EQUIP_SLOT_COUNT]) {
    hovered_inv_slot = -1;
    hovered_equip_slot = -1;

    for (size_t i = 0; i < slots.size() && i < static_cast<size_t>(cfg.equip_slots); ++i) {
        int col = static_cast<int>(i) % cfg.cols;
        int row = static_cast<int>(i) / cfg.cols;
        int sx = cfg.x + col * (cfg.slot_w + cfg.gap);
        int sy = cfg.y + row * (cfg.slot_h + cfg.gap);
        if (point_in_rect(mx, my, SDL2pp::Rect(sx, sy, cfg.slot_w, cfg.slot_h))) {
            if (slots[i].item_type != ItemType::NONE) {
                hovered_inv_slot = static_cast<int>(i);
            }
            return;
        }
    }

    for (int i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        int sx = cfg.x + i * (cfg.slot_w + cfg.gap);
        int sy = cfg.equip_y;
        if (point_in_rect(mx, my, SDL2pp::Rect(sx, sy, cfg.slot_w, cfg.slot_h))) {
            if (equipped[i].item_type != ItemType::NONE) {
                hovered_equip_slot = i;
            }
            return;
        }
    }
}

bool InventoryRenderer::is_hovering_occupied() const {
    return hovered_inv_slot >= 0 || hovered_equip_slot >= 0;
}

void InventoryRenderer::draw_hover_border(int sx, int sy) {
    renderer.SetDrawColor(220, 180, 40, 255);
    SDL2pp::Rect border(sx, sy, cfg.slot_w, cfg.slot_h);
    renderer.DrawRect(border);
    SDL2pp::Rect inner(sx + 1, sy + 1, cfg.slot_w - 2, cfg.slot_h - 2);
    renderer.DrawRect(inner);
}

void InventoryRenderer::render_item_tooltip(int sx, int sy, const InventorySlot& slot) {
    if (!font || slot.item_type == ItemType::NONE)
        return;

    const ItemSpriteDef* def = find_sprite(slot.item_type);
    std::string text = slot.item_name;
    if (def && !def->category.empty()) {
        text += "\n" + def->category;
    }

    SDL_Color white{255, 255, 255, 255};
    auto result = texture::render_text(renderer, font, text, white);
    if (result.w == 0)
        return;

    const int pad = 6;
    int tw = result.w + pad * 2;
    int th = result.h + pad * 2;
    int tx = sx + cfg.slot_w + 6;
    int ty = sy;

    if (tx + tw > renderer.GetLogicalWidth()) {
        tx = sx - tw - 6;
    }
    if (ty + th > renderer.GetLogicalHeight()) {
        ty = renderer.GetLogicalHeight() - th - 2;
    }
    if (tx < 0)
        tx = 2;

    SDL2pp::Rect bg(tx, ty, tw, th);
    renderer.SetDrawColor(0, 0, 0, 220);
    renderer.FillRect(bg);
    renderer.SetDrawColor(180, 180, 180, 255);
    renderer.DrawRect(bg);

    SDL2pp::Rect text_dst(tx + pad, ty + pad, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void InventoryRenderer::render(const std::vector<InventorySlot>& slots) {
    for (size_t i = 0; i < slots.size() && i < static_cast<size_t>(cfg.equip_slots); ++i) {
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
            render_item_sprite(slots[i].item_type, slots[i].item_name, slot_rect);
        }
    }

    if (hovered_inv_slot >= 0 && static_cast<size_t>(hovered_inv_slot) < slots.size() &&
        slots[hovered_inv_slot].item_type != ItemType::NONE) {
        int col = hovered_inv_slot % cfg.cols;
        int row = hovered_inv_slot / cfg.cols;
        int sx = cfg.x + col * (cfg.slot_w + cfg.gap);
        int sy = cfg.y + row * (cfg.slot_h + cfg.gap);
        draw_hover_border(sx, sy);
        render_item_tooltip(sx, sy, slots[hovered_inv_slot]);
    }
}

void InventoryRenderer::render_equipped(const InventorySlot equipped[EQUIP_SLOT_COUNT]) {
    const char* labels[EQUIP_SLOT_COUNT] = {
            cfg.equip_weapon_label.c_str(), cfg.equip_armor_label.c_str(),
            cfg.equip_helmet_label.c_str(), cfg.equip_shield_label.c_str()};

    for (int i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        int sx = cfg.x + i * (cfg.slot_w + cfg.gap);
        int sy = cfg.equip_y;
        SDL2pp::Rect slot_rect(sx, sy, cfg.slot_w, cfg.slot_h);

        if (equipped[i].item_type == ItemType::NONE) {
            renderer.SetDrawColor(40, 40, 40, 255);
            renderer.FillRect(slot_rect);
            renderer.SetDrawColor(80, 80, 80, 255);
            renderer.DrawRect(slot_rect);
        } else {
            render_item_sprite(equipped[i].item_type, "", slot_rect);
        }

        if (font) {
            auto result = texture::render_text(renderer, font, labels[i], {255, 255, 255, 255});
            if (result.w > 0) {
                int tx = sx + (cfg.slot_w - result.w) / 2;
                int ty = sy + cfg.slot_h + 2;
                SDL2pp::Rect text_dst(tx, ty, result.w, result.h);
                renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
            }
        }
    }

    if (hovered_equip_slot >= 0 && equipped[hovered_equip_slot].item_type != ItemType::NONE) {
        int sx = cfg.x + hovered_equip_slot * (cfg.slot_w + cfg.gap);
        int sy = cfg.equip_y;
        draw_hover_border(sx, sy);
        render_item_tooltip(sx, sy, equipped[hovered_equip_slot]);
    }
}
