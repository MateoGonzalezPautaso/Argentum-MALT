#ifndef CLIENT_UI_RENDERER_H
#define CLIENT_UI_RENDERER_H

#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../../common/messages.h"
#include "../../chat/chat_history.h"
#include "../../config/config.h"
#include "../../render/gfx/button.h"

#include "inventory_renderer.h"

class ChatInput;

class UIRenderer {
private:
    SDL2pp::Renderer& renderer;
    const ChatInput& chat_model;
    const SkinConfig& skin_config;
    SDL2pp::Texture ui_frame_texture;
    SDL2pp::Texture hp_bar_texture;
    SDL2pp::Texture mp_bar_texture;
    SDL2pp::Texture exp_bar_texture;
    Button audio_button;
    SDL2pp::Texture audio_off_texture;
    bool audio_muted_ = false;
    Button expand_button;
    SDL2pp::Texture expand_active_texture;
    SDL2pp::Texture big_chat_texture_;
    bool chat_expanded_ = false;
    int expanded_input_y_ = 0;
    SDL2pp::Rect ui_frame_rect;
    SDL2pp::Rect chat_input_rect;
    SDL2pp::Rect chat_history_rect;
    SDL2pp::Rect expanded_chat_history_rect;
    TTF_Font* chat_font = nullptr;
    TTF_Font* bar_font = nullptr;
    SDL_Color chat_color{255, 255, 255, 255};
    UIConfig ui_cfg;
    InventoryRenderer inventory_renderer;
    int hovered_potion = 0;
    int hp_potion_count_ = 0;
    int mana_potion_count_ = 0;
    int first_hp_potion_slot_ = -1;
    int first_mana_potion_slot_ = -1;

public:
    UIRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg, const SkinConfig& skin_config,
               const ChatInput& chat_model,
               const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites);
    ~UIRenderer();

    void render_ui_frame();
    void render_audio_button();
    bool is_audio_hit(int x, int y) const;
    void set_audio_button_hovered(int x, int y);
    void set_audio_muted(bool muted) { audio_muted_ = muted; }
    void render_chat_input();
    void render_expand_button();
    bool is_expand_hit(int x, int y) const;
    void set_expand_button_hovered(int x, int y);
    void set_chat_expanded(bool expanded);
    bool is_chat_expanded() const { return chat_expanded_; }
    void render_chat_history(const std::vector<ChatMessage>& messages, int scroll_offset = 0);
    void render_hp_bar(uint32_t current, uint32_t max);
    void render_mp_bar(uint32_t current, uint32_t max);
    void render_exp_bar(uint32_t current, uint32_t max);
    void render_gold(uint32_t gold);
    void render_crit_chance(uint8_t crit_pct);
    void render_dodge_chance(uint8_t dodge_pct);
    void render_strength(uint16_t strength);
    void render_agility(uint16_t agility);
    void render_damage(uint16_t dmg_min, uint16_t dmg_max);
    void render_defense(uint16_t def_min, uint16_t def_max);
    void update_potion_button_hover(int mx, int my, const std::vector<InventorySlot>& slots);
    void render_potion_buttons();
    int get_hovered_potion() const { return hovered_potion; }
    int get_first_hp_potion_slot() const { return first_hp_potion_slot_; }
    int get_first_mana_potion_slot() const { return first_mana_potion_slot_; }
    void render_portrait(Race race, PlayerClass player_class, uint8_t level);
    void set_hover(int mx, int my, const std::vector<InventorySlot>& slots,
                   const InventorySlot equipped[EQUIP_SLOT_COUNT]);
    bool is_hovering_occupied() const;
    int get_hovered_inv_slot() const;
    int get_hovered_equip_slot() const;
    void render_inventory(const std::vector<InventorySlot>& slots);
    void render_equipped(const InventorySlot equipped[EQUIP_SLOT_COUNT]);
    bool is_chat_input_hit(int x, int y) const;
    void render_stat_tooltips(int mx, int my) const;

private:
    void render_stat_bar(SDL2pp::Texture& tex, int x, int y, int w, int h, uint32_t current,
                         uint32_t max) const;
    void render_centered_text(const StatBarConfig& cfg, const std::string& text) const;
    std::vector<const StatBarConfig*> all_stat_bar_configs() const;
    void render_chat_cursor(int x_offset) const;
    void render_chat_text_line(int& clipped_w) const;
    std::vector<std::string> wrap_chat_text(const std::string& text, int max_width) const;
};

#endif
