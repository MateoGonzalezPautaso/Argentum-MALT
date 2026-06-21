#include "ui_renderer.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "../input/chat_input.h"

#include "geometry.h"
#include "text_renderer.h"
#include "texture_loader.h"

UIRenderer::UIRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                       const SkinConfig& skin_config, const ChatInput& chat_model,
                       const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites):
        renderer(renderer),
        chat_model(chat_model),
        skin_config(skin_config),
        ui_frame_texture(renderer, texture::load_surface(ui_cfg.asset_ui_frame)),
        hp_bar_texture(renderer, texture::load_surface(ui_cfg.asset_hp_bar)),
        mp_bar_texture(renderer, texture::load_surface(ui_cfg.asset_mp_bar)),
        exp_bar_texture(renderer, texture::load_surface(ui_cfg.asset_exp_bar)),
        audio_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_audio_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_audio_hover))),
        audio_off_texture(renderer, texture::load_surface(ui_cfg.asset_audio_off)),
        expand_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_expand_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_expand_hover))),
        expand_active_texture(renderer, texture::load_surface(ui_cfg.asset_expand_off)),
        big_chat_texture_(renderer, texture::load_surface(ui_cfg.asset_big_chat)),
        ui_frame_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        chat_input_rect(ui_cfg.chat_input_x, ui_cfg.chat_input_y, ui_cfg.chat_input_w,
                        ui_cfg.chat_input_h),
        chat_history_rect(ui_cfg.chat_history_x, ui_cfg.chat_history_y, ui_cfg.chat_history_w,
                          ui_cfg.chat_history_h),
        ui_cfg(ui_cfg),
        inventory_renderer(renderer, bar_font, ui_cfg.inventory_panel, item_sprites) {
    chat_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_chat_size);
    if (!chat_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    bar_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_bar_size);
    if (!bar_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    inventory_renderer.set_font(bar_font);
    audio_button.set_position(ui_cfg.game_audio_x, ui_cfg.game_audio_y,
                              ui_cfg.game_audio_w, ui_cfg.game_audio_h);
    const int expand_x = ui_cfg.chat_input_x + ui_cfg.chat_input_w + ui_cfg.chat_expand_btn_gap;
    expand_button.set_position(expand_x, ui_cfg.chat_input_y,
                               ui_cfg.chat_expand_btn_w, ui_cfg.chat_expand_btn_h);

    const int orig_gap = ui_cfg.chat_input_y - (ui_cfg.chat_history_y + ui_cfg.chat_history_h);
    const int img_bottom = ui_cfg.chat_history_y + big_chat_texture_.GetHeight();
    expanded_input_y_ = img_bottom - ui_cfg.chat_input_h - orig_gap;
    expanded_chat_history_rect = SDL2pp::Rect(
            ui_cfg.chat_history_x, ui_cfg.chat_history_y, ui_cfg.chat_history_w,
            expanded_input_y_ - orig_gap - ui_cfg.chat_history_y);
}

UIRenderer::~UIRenderer() {
    if (chat_font) {
        TTF_CloseFont(chat_font);
        chat_font = nullptr;
    }
    if (bar_font) {
        TTF_CloseFont(bar_font);
        bar_font = nullptr;
    }
}

void UIRenderer::render_frame_background() {
    renderer.Copy(ui_frame_texture, SDL2pp::NullOpt, ui_frame_rect);
}

void UIRenderer::render_audio_button() {
    audio_button.render_togglable(renderer, audio_off_texture, audio_muted_);
}

bool UIRenderer::is_audio_hit(int x, int y) const { return audio_button.is_hit(x, y); }

void UIRenderer::set_audio_button_hovered(int x, int y) {
    audio_button.hovered = audio_button.is_hit(x, y);
}

void UIRenderer::render_chat_input() {
    if (!chat_font) {
        return;
    }

    const bool show_cursor = chat_model.is_focused() && ((SDL_GetTicks() / 500U) % 2U == 0U);

    int clipped_w = 0;
    const std::string& text = chat_model.get_text();
    if (!text.empty()) {
        render_chat_text_line(clipped_w);
    }

    if (show_cursor) {
        render_chat_cursor(clipped_w);
    }
}

bool UIRenderer::is_chat_input_hit(int x, int y) const {
    return point_in_rect(x, y, chat_input_rect);
}

void UIRenderer::render_expand_button() {
    expand_button.render_togglable(renderer, expand_active_texture, chat_expanded_);
}

bool UIRenderer::is_expand_hit(int x, int y) const {
    return expand_button.is_hit(x, y);
}

void UIRenderer::set_expand_button_hovered(int x, int y) {
    expand_button.hovered = expand_button.is_hit(x, y);
}

void UIRenderer::set_chat_expanded(bool expanded) {
    chat_expanded_ = expanded;
    const int input_y = expanded ? expanded_input_y_ : ui_cfg.chat_input_y;
    chat_input_rect = SDL2pp::Rect(ui_cfg.chat_input_x, input_y,
                                   ui_cfg.chat_input_w, ui_cfg.chat_input_h);
    const int expand_btn_x = ui_cfg.chat_input_x + ui_cfg.chat_input_w + ui_cfg.chat_expand_btn_gap;
    expand_button.rect = SDL2pp::Rect(expand_btn_x, input_y,
                                      ui_cfg.chat_expand_btn_w, ui_cfg.chat_expand_btn_h);
}

void UIRenderer::render_hp_bar(uint32_t current, uint32_t max) {
    render_stat_bar(hp_bar_texture, ui_cfg.hp_bar.x, ui_cfg.hp_bar.y, ui_cfg.hp_bar.w,
                    ui_cfg.hp_bar.h, current, max);
}

void UIRenderer::render_mp_bar(uint32_t current, uint32_t max) {
    render_stat_bar(mp_bar_texture, ui_cfg.mp_bar.x, ui_cfg.mp_bar.y, ui_cfg.mp_bar.w,
                    ui_cfg.mp_bar.h, current, max);
}

void UIRenderer::render_exp_bar(uint32_t current, uint32_t max) {
    render_stat_bar(exp_bar_texture, ui_cfg.exp_bar.x, ui_cfg.exp_bar.y, ui_cfg.exp_bar.w,
                    ui_cfg.exp_bar.h, current, max);
}

void UIRenderer::render_stat_bar(SDL2pp::Texture& tex, int x, int y, int w, int h, uint32_t current,
                                 uint32_t max) const {
    if (max == 0) {
        return;
    }

    const float ratio = std::min(1.0f, static_cast<float>(current) / static_cast<float>(max));
    const int filled_w = static_cast<int>(w * ratio);

    if (filled_w > 0) {
        SDL2pp::Rect src(0, 0, filled_w, h);
        SDL2pp::Rect dst(x, y, filled_w, h);
        renderer.Copy(tex, src, dst);
    }

    if (!bar_font) {
        return;
    }

    std::string text = std::to_string(current) + "/" + std::to_string(max);
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = x + (w - result.w) / 2;
    const int text_y = y + (h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_gold(uint32_t gold) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.gold_rect;
    std::string text = std::to_string(gold);
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_crit_chance(uint8_t crit_pct) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.crit_rect;
    std::string text = std::to_string(crit_pct) + "%";
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_dodge_chance(uint8_t dodge_pct) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.dodge_rect;
    std::string text = std::to_string(dodge_pct) + "%";
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_strength(uint16_t strength) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.strength_rect;
    std::string text = std::to_string(strength);
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_agility(uint16_t agility) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.agility_rect;
    std::string text = std::to_string(agility);
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_damage(uint16_t dmg_min, uint16_t dmg_max) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.damage_rect;
    std::string text = std::to_string(dmg_min) + "-" + std::to_string(dmg_max);
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::render_defense(uint16_t def_min, uint16_t def_max) {
    if (!bar_font) {
        return;
    }

    const auto& gr = ui_cfg.defense_rect;
    std::string text = std::to_string(def_min) + "-" + std::to_string(def_max);
    auto result = texture::render_text(renderer, bar_font, text, chat_color);
    if (result.w == 0) {
        return;
    }
    const int text_x = gr.x + (gr.w - result.w) / 2;
    const int text_y = gr.y + (gr.h - result.h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
}

void UIRenderer::update_potion_button_hover(int mx, int my,
                                            const std::vector<InventorySlot>& slots) {
    hovered_potion = 0;
    hp_potion_count_ = 0;
    mana_potion_count_ = 0;
    first_hp_potion_slot_ = -1;
    first_mana_potion_slot_ = -1;

    for (const auto& s: slots) {
        if (s.item_type == ItemType::HEALTH_POTION) {
            if (first_hp_potion_slot_ < 0)
                first_hp_potion_slot_ = s.slot_index;
            ++hp_potion_count_;
        }
        if (s.item_type == ItemType::MANA_POTION) {
            if (first_mana_potion_slot_ < 0)
                first_mana_potion_slot_ = s.slot_index;
            ++mana_potion_count_;
        }
    }

    if (hp_potion_count_ > 0) {
        SDL2pp::Rect r(ui_cfg.potion_hp.x, ui_cfg.potion_hp.y, ui_cfg.potion_hp.w,
                       ui_cfg.potion_hp.h);
        if (point_in_rect(mx, my, r))
            hovered_potion = 1;
    }
    if (mana_potion_count_ > 0) {
        SDL2pp::Rect r(ui_cfg.potion_mana.x, ui_cfg.potion_mana.y, ui_cfg.potion_mana.w,
                       ui_cfg.potion_mana.h);
        if (point_in_rect(mx, my, r))
            hovered_potion = 2;
    }
}

void UIRenderer::render_potion_buttons() {
    if (!bar_font)
        return;

    const auto draw_button = [this](const StatBarConfig& btn, int count) {
        std::string text = std::to_string(count);
        auto result = texture::render_text(renderer, bar_font, text, chat_color);
        if (result.w == 0)
            return;
        SDL2pp::Rect r(btn.x, btn.y, btn.w, btn.h);
        const int tx = r.GetX() + (r.GetW() - result.w) / 2;
        const int ty = r.GetY() + (r.GetH() - result.h) / 2;
        SDL2pp::Rect text_dst(tx, ty, result.w, result.h);
        renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
    };

    draw_button(ui_cfg.potion_hp, hp_potion_count_);
    draw_button(ui_cfg.potion_mana, mana_potion_count_);

    if (hovered_potion == 0)
        return;

    const StatBarConfig& btn = (hovered_potion == 1) ? ui_cfg.potion_hp : ui_cfg.potion_mana;
    SDL2pp::Rect r(btn.x, btn.y, btn.w, btn.h);
    renderer.SetDrawColor(220, 180, 40, 255);
    renderer.DrawRect(r);
    SDL2pp::Rect inner(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2);
    renderer.DrawRect(inner);
}

void UIRenderer::render_portrait(Race race, PlayerClass player_class, uint8_t level) {
    const auto& pt = ui_cfg.portrait;

    std::string head_path = skin_config.head_path_for(race);
    std::string body_path = skin_config.body_path_for(player_class);

    const float scale = pt.h * pt.zoom / pt.head_src_h;

    const int head_dst_w = static_cast<int>(pt.head_src_w * scale);
    const int head_dst_h = static_cast<int>(pt.head_src_h * scale);
    const int body_dst_w = static_cast<int>(pt.body_src_w * scale);
    const int body_dst_h = static_cast<int>(pt.body_shoulder_h * scale);

    const int head_x = pt.x + (pt.w - head_dst_w) / 2;
    const int body_x = head_x;
    const int body_y = pt.y + pt.h - body_dst_h;
    const int head_y = body_y + static_cast<int>(pt.anchor_y * scale);

    if (!head_path.empty()) {
        SDL2pp::Surface surf = texture::load_surface(head_path);
        SDL2pp::Texture tex(renderer, surf);
        SDL2pp::Rect src(0, 0, pt.head_src_w, pt.head_src_h);
        SDL2pp::Rect dst(head_x, head_y, head_dst_w, head_dst_h);
        renderer.Copy(tex, src, dst);
    }

    if (!body_path.empty()) {
        SDL2pp::Surface surf = texture::load_surface(body_path);
        SDL2pp::Texture tex(renderer, surf);
        SDL2pp::Rect src(0, 0, pt.body_src_w, pt.body_shoulder_h);
        SDL2pp::Rect dst(body_x, body_y, body_dst_w, body_dst_h);
        renderer.Copy(tex, src, dst);
    }

    if (chat_font) {
        std::string lvl_text = "lvl " + std::to_string(level);
        SDL_Color lvl_color{255, 255, 0, 255};
        auto result = texture::render_text(renderer, chat_font, lvl_text, lvl_color);
        if (result.w > 0) {
            const int text_x = pt.lvl_x - result.w;
            const int text_y = pt.lvl_y - result.h;
            SDL2pp::Rect text_dst(text_x, text_y, result.w, result.h);
            renderer.Copy(result.texture, SDL2pp::NullOpt, text_dst);
        }
    }
}

void UIRenderer::set_hover(int mx, int my, const std::vector<InventorySlot>& slots,
                           const InventorySlot equipped[4]) {
    inventory_renderer.update_hover(mx, my, slots, equipped);
}

bool UIRenderer::is_hovering_occupied() const { return inventory_renderer.is_hovering_occupied(); }

int UIRenderer::get_hovered_inv_slot() const { return inventory_renderer.get_hovered_inv_slot(); }

int UIRenderer::get_hovered_equip_slot() const {
    return inventory_renderer.get_hovered_equip_slot();
}

void UIRenderer::render_inventory(const std::vector<InventorySlot>& slots) {
    inventory_renderer.render(slots);
}

void UIRenderer::render_equipped(const InventorySlot equipped[4]) {
    inventory_renderer.render_equipped(equipped);
}

std::vector<std::string> UIRenderer::wrap_chat_text(const std::string& text, int max_width) const {
    std::vector<std::string> lines;
    if (!chat_font || max_width <= 0) {
        lines.push_back(text);
        return lines;
    }

    std::string current_line;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t space_pos = text.find(' ', pos);
        std::string word = text.substr(pos, space_pos == std::string::npos
                                                    ? std::string::npos
                                                    : space_pos - pos);

        std::string candidate = current_line.empty() ? word : current_line + " " + word;
        int w = 0, h = 0;
        TTF_SizeUTF8(chat_font, candidate.c_str(), &w, &h);

        if (w > max_width && !current_line.empty()) {
            lines.push_back(current_line);
            current_line = word;
        } else {
            current_line = candidate;
        }

        if (space_pos == std::string::npos)
            break;
        pos = space_pos + 1;
    }
    if (!current_line.empty() || lines.empty())
        lines.push_back(current_line);

    return lines;
}

void UIRenderer::render_chat_history(const std::vector<ChatMessage>& messages, int scroll_offset) {
    const SDL2pp::Rect& hist_rect = chat_expanded_ ? expanded_chat_history_rect : chat_history_rect;

    if (chat_expanded_) {
        renderer.Copy(big_chat_texture_, SDL2pp::NullOpt,
                      SDL2pp::Rect(ui_cfg.chat_expand_x, chat_history_rect.GetY(),
                                   big_chat_texture_.GetWidth(), big_chat_texture_.GetHeight()));
    }

    if (!chat_font || messages.empty()) {
        return;
    }

    int y_offset = hist_rect.GetY() + hist_rect.GetH();
    int line_spacing = ui_cfg.chat_line_spacing;

    int skipped = 0;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (skipped < scroll_offset) {
            ++skipped;
            continue;
        }
        std::string display;
        SDL_Color color = chat_color;

        switch (it->type) {
            case ChatMsgType::PRIVATE:
                display = "[Priv] " + it->sender + ": " + it->text;
                color = {255, 255, 0, 255};
                break;
            case ChatMsgType::CLAN:
                display = "[Clan] " + it->sender + ": " + it->text;
                color = {0, 255, 0, 255};
                break;
            case ChatMsgType::SYSTEM:
                display = it->text;
                color = {192, 192, 192, 255};
                break;
            default:
                display = it->sender.empty() ? it->text : it->sender + ": " + it->text;
                break;
        }

        std::vector<std::string> wrapped_lines = wrap_chat_text(display, hist_rect.GetW());
        bool stop = false;
        for (auto line_it = wrapped_lines.rbegin(); line_it != wrapped_lines.rend(); ++line_it) {
            auto result = texture::render_text(renderer, chat_font, *line_it, color);
            if (result.w == 0) {
                continue;
            }

            y_offset -= (result.h + line_spacing);
            if (y_offset < hist_rect.GetY()) {
                stop = true;
                break;
            }

            int clipped_w = std::min(result.w, hist_rect.GetW());
            SDL2pp::Rect src_rect(0, 0, clipped_w, result.h);
            SDL2pp::Rect dst_rect(hist_rect.GetX(), y_offset, clipped_w, result.h);
            renderer.Copy(result.texture, src_rect, dst_rect);
        }
        if (stop) {
            break;
        }
    }
}

void UIRenderer::render_chat_text_line(int& clipped_w) const {
    clipped_w = texture::render_text_clipped(renderer, chat_font, chat_model.get_text(), chat_color,
                                             chat_input_rect);
}

void UIRenderer::render_chat_cursor(int x_offset) const {
    auto result = texture::render_text(renderer, chat_font, "|", chat_color);
    if (result.w == 0) {
        return;
    }
    const int cursor_x = std::min(chat_input_rect.GetX() + x_offset,
                                  chat_input_rect.GetX() + chat_input_rect.GetW() - result.w);
    SDL2pp::Rect cursor_dst(cursor_x, chat_input_rect.GetY(),
                            std::min(result.w, chat_input_rect.GetW()),
                            std::min(result.h, chat_input_rect.GetH()));
    renderer.Copy(result.texture, SDL2pp::NullOpt, cursor_dst);
}

void UIRenderer::render_stat_tooltips(int mx, int my) const {
    if (!bar_font)
        return;

    const StatBarConfig* all_stats[] = {&ui_cfg.hp_bar,   &ui_cfg.mp_bar,       &ui_cfg.exp_bar,
                                        &ui_cfg.gold_rect, &ui_cfg.potion_hp,    &ui_cfg.potion_mana,
                                        &ui_cfg.crit_rect, &ui_cfg.dodge_rect,   &ui_cfg.strength_rect,
                                        &ui_cfg.agility_rect, &ui_cfg.damage_rect, &ui_cfg.defense_rect};

    const StatBarConfig* found = nullptr;
    for (const auto* stat : all_stats) {
        if (stat->label.empty())
            continue;
        if (point_in_rect(mx, my, SDL2pp::Rect(stat->x, stat->y, stat->w, stat->h))) {
            found = stat;
            break;
        }
    }

    if (!found)
        return;

    auto result = texture::render_text(renderer, bar_font, found->label, {255, 255, 150, 255});
    if (result.w == 0)
        return;

    SDL2pp::Rect dst(mx + 12, my + 12, result.w, result.h);
    if (dst.GetX() + dst.GetW() > ui_cfg.window_w)
        dst.SetX(ui_cfg.window_w - dst.GetW());
    if (dst.GetY() + dst.GetH() > ui_cfg.window_h)
        dst.SetY(ui_cfg.window_h - dst.GetH());
    if (dst.GetX() < 0)
        dst.SetX(0);
    if (dst.GetY() < 0)
        dst.SetY(0);
    renderer.Copy(result.texture, SDL2pp::NullOpt, dst);
}
