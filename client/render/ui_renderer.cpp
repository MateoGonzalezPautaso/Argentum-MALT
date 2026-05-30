#include "ui_renderer.h"

#include <algorithm>
#include <string>

#include "../input/chat_input.h"

#include "geometry.h"
#include "text_renderer.h"
#include "texture_loader.h"

UIRenderer::UIRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                       const SkinConfig& skin_config, const ChatInput& chat_model):
        renderer(renderer),
        chat_model(chat_model),
        skin_config(skin_config),
        ui_frame_texture(renderer, texture::load_surface(ui_cfg.asset_ui_frame)),
        hp_bar_texture(renderer, texture::load_surface(ui_cfg.asset_hp_bar)),
        mp_bar_texture(renderer, texture::load_surface(ui_cfg.asset_mp_bar)),
        exp_bar_texture(renderer, texture::load_surface(ui_cfg.asset_exp_bar)),
        ui_frame_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        chat_input_rect(ui_cfg.chat_input_x, ui_cfg.chat_input_y, ui_cfg.chat_input_w,
                        ui_cfg.chat_input_h),
        chat_history_rect(ui_cfg.chat_history_x, ui_cfg.chat_history_y, ui_cfg.chat_history_w,
                          ui_cfg.chat_history_h),
        ui_cfg(ui_cfg),
        inventory_renderer(renderer, bar_font, ui_cfg.inventory_panel) {
    chat_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_chat_size);
    if (!chat_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    bar_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_bar_size);
    if (!bar_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    inventory_renderer.set_font(bar_font);
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

void UIRenderer::render_inventory(const std::vector<InventorySlot>& slots) {
    inventory_renderer.render(slots);
}

void UIRenderer::render_chat_history(const std::vector<ChatMessage>& messages) {
    if (!chat_font || messages.empty()) {
        return;
    }

    int y_offset = chat_history_rect.GetY() + chat_history_rect.GetH();
    int line_spacing = ui_cfg.chat_line_spacing;

    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
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

        auto result = texture::render_text(renderer, chat_font, display, color);
        if (result.w == 0) {
            continue;
        }

        y_offset -= (result.h + line_spacing);
        if (y_offset < chat_history_rect.GetY()) {
            break;
        }

        int clipped_w = std::min(result.w, chat_history_rect.GetW());
        SDL2pp::Rect src_rect(0, 0, clipped_w, result.h);
        SDL2pp::Rect dst_rect(chat_history_rect.GetX(), y_offset, clipped_w, result.h);
        renderer.Copy(result.texture, src_rect, dst_rect);
    }
}

void UIRenderer::render_chat_text_line(int& clipped_w) const {
    clipped_w = texture::render_text_clipped(renderer, chat_font, chat_model.get_text(),
                                              chat_color, chat_input_rect);
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
