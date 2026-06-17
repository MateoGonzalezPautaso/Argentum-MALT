#include "merchant_controller.h"

#include "../render/geometry.h"

MerchantController::MerchantController(SDL2pp::Renderer& renderer, const UIConfig& cfg,
                                       Queue<ClientCommand>& command_queue,
                                       const PlayerStats& player_stats):
        renderer_(renderer),
        merchant_renderer_(renderer, cfg),
        player_stats_(player_stats),
        command_queue_(command_queue) {}

void MerchantController::open() {
    open_ = true;
    command_queue_.push(NpcListCmd{});
}

bool MerchantController::is_open() const {
    return open_;
}

bool MerchantController::is_any_button_hovered() const {
    return merchant_renderer_.is_any_button_hovered();
}

void MerchantController::close() {
    open_ = false;
    items_.clear();
    selected_idx_ = -1;
    merchant_renderer_.set_items({});
    merchant_renderer_.set_selected(-1);
}

void MerchantController::on_item_list(const NpcItemListEvent& ev) {
    items_ = ev.items;
    selected_idx_ = ev.items.empty() ? -1 : 0;
    merchant_renderer_.set_items(ev.items);
    merchant_renderer_.set_selected(selected_idx_);
}

void MerchantController::render() {
    merchant_renderer_.render(renderer_, player_stats_.gold);
}

bool MerchantController::handle_mouse_button(const SDL_Event& event) {
    if (!open_)
        return false;

    const SDL2pp::Rect panel = merchant_renderer_.panel_bounds();
    if (!point_in_rect(event.button.x, event.button.y, panel)) {
        close();
        return true;
    }

    int clicked_idx = merchant_renderer_.item_at(event.button.x, event.button.y);
    if (clicked_idx >= 0) {
        selected_idx_ = clicked_idx;
        merchant_renderer_.set_selected(clicked_idx);
        return true;
    }

    if (merchant_renderer_.is_buy_hit(event.button.x, event.button.y)) {
        if (selected_idx_ >= 0 && selected_idx_ < static_cast<int>(items_.size()))
            command_queue_.push(NpcBuyCmd{items_[selected_idx_].item_name});
        return true;
    }

    if (merchant_renderer_.is_sell_hit(event.button.x, event.button.y)) {
        if (selected_idx_ >= 0 && selected_idx_ < static_cast<int>(items_.size()))
            command_queue_.push(NpcSellCmd{items_[selected_idx_].item_name});
        return true;
    }

    return true;
}

bool MerchantController::handle_mouse_motion(int x, int y) {
    if (!open_)
        return false;

    merchant_renderer_.set_buy_hovered(x, y);
    merchant_renderer_.set_sell_hovered(x, y);
    merchant_renderer_.set_plus_hovered(x, y);
    merchant_renderer_.set_minus_hovered(x, y);
    return true;
}

bool MerchantController::handle_keydown(const SDL_Event& event) {
    if (!open_)
        return false;

    if (event.key.keysym.sym == SDLK_ESCAPE) {
        close();
        return true;
    }

    return true;
}
