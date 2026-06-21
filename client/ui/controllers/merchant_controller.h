#ifndef CLIENT_MERCHANT_CONTROLLER_H
#define CLIENT_MERCHANT_CONTROLLER_H

#include <vector>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../../../common/messages.h"
#include "../../../common/queue.h"
#include "../../config/config.h"
#include "../../config/player_stats.h"
#include "../hud/merchant_renderer.h"

class MerchantController {
public:
    MerchantController(SDL2pp::Renderer& renderer, const UIConfig& cfg,
                       Queue<ClientCommand>& command_queue, const PlayerStats& player_stats);

    void open(bool sell_enabled = true);
    bool is_open() const;
    bool is_any_button_hovered() const;

    bool handle_mouse_button(const SDL_Event& event);
    bool handle_mouse_motion(int x, int y);
    bool handle_keydown(const SDL_Event& event);
    void handle_scroll(int delta);

    void on_item_list(const NpcItemListEvent& ev);
    void render();

private:
    SDL2pp::Renderer& renderer_;
    MerchantRenderer merchant_renderer_;
    const PlayerStats& player_stats_;
    Queue<ClientCommand>& command_queue_;
    bool open_ = false;
    std::vector<NpcItemEntry> items_;
    int selected_idx_ = -1;
    int scroll_offset_ = 0;

    void close();
};

#endif  // CLIENT_MERCHANT_CONTROLLER_H
