#include "game_controller.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "../render/gfx/geometry.h"

GameController::GameController(SDL2pp::Renderer& renderer, const ClientConfig& config,
                               Queue<ClientCommand>& command_queue, AudioManager& audio_manager):
        audio_manager(audio_manager),
        config(config),
        renderer(renderer),
        world_renderer(renderer, config.background, config.viewport, config.font),
        ui_renderer(renderer, config.ui, config.skins, chat_input, config.item_sprites),
        command_queue(command_queue),
        move_controller(this->command_queue, MoveConfig(config), SDL_GetTicks()),
        move_config(config),
        hand_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)),
        arrow_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW)),
        cheat_commands_{
                {SDLK_h, ClientCommand{CheatInfiniteHpCmd{}}},
                {SDLK_i, ClientCommand{CheatFillInventoryCmd{}}},
                {SDLK_m, ClientCommand{CheatInfiniteManaCmd{}}},
                {SDLK_k, ClientCommand{CheatDieCmd{}}},
                {SDLK_l, ClientCommand{CheatClearInventoryCmd{}}},
                {SDLK_v, ClientCommand{CheatLevelUpCmd{}}},
                {SDLK_b, ClientCommand{CheatLevelDownCmd{}}},
                {SDLK_g, ClientCommand{CheatAddGoldCmd{}}},
                {SDLK_0, ClientCommand{CheatResetGoldCmd{}}},
                {SDLK_9, ClientCommand{CheatResetManaCmd{}}},
                {SDLK_f, ClientCommand{CheatVelocityCmd{}}},
                {SDLK_r, ClientCommand{CheatReviveCmd{}}},
        },
        merchant_controller(std::make_unique<MerchantController>(renderer, config.ui, command_queue,
                                                                 player_stats)),
        event_handler_(player_stats, world_renderer, audio_manager, chat_history, move_controller,
                       config, move_config, *merchant_controller, player_is_ghost,
                       current_map_name) {
    world_renderer.sprites().set_direction_src_y(config.dir_src_y_down, config.dir_src_y_up,
                                                 config.dir_src_y_left, config.dir_src_y_right);
}

GameController::~GameController() {
    SDL_FreeCursor(hand_cursor);
    SDL_FreeCursor(arrow_cursor);
}

void GameController::tick() {
    uint32_t now = SDL_GetTicks();
    if (auto dir = move_controller.tick(now)) {
        apply_movement_visual(*dir, true);
    }
}

void GameController::render() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    ui_renderer.render_ui_frame();
    world_renderer.render();
    ui_renderer.render_portrait(player_stats.race, player_stats.player_class, player_stats.level);
    ui_renderer.render_gold(player_stats.gold);
    ui_renderer.render_crit_chance(player_stats.crit_chance);
    ui_renderer.render_dodge_chance(player_stats.dodge_chance);
    ui_renderer.render_strength(player_stats.strength);
    ui_renderer.render_agility(player_stats.agility);
    ui_renderer.render_damage(player_stats.damage_min, player_stats.damage_max);
    ui_renderer.render_defense(player_stats.defense_min, player_stats.defense_max);
    ui_renderer.render_potion_buttons();
    ui_renderer.render_hp_bar(player_stats.hp_current, player_stats.hp_max);
    ui_renderer.render_mp_bar(player_stats.mana_current, player_stats.mana_max);
    ui_renderer.render_exp_bar(player_stats.experience, player_stats.exp_to_next);
    if (chat_history.has_new_message() && chat_scroll == 0)
        chat_history.consume_new_message();
    ui_renderer.render_chat_history(chat_history.get_messages(), chat_scroll);
    ui_renderer.render_chat_input();
    ui_renderer.render_expand_button();
    ui_renderer.set_hover(mouse_x, mouse_y, player_stats.inventory, player_stats.equipped);
    ui_renderer.update_potion_button_hover(mouse_x, mouse_y, player_stats.inventory);
    ui_renderer.render_inventory(player_stats.inventory);
    ui_renderer.render_equipped(player_stats.equipped);
    ui_renderer.render_stat_tooltips(mouse_x, mouse_y);
    ui_renderer.set_audio_muted(audio_manager.is_muted());
    ui_renderer.render_audio_button();
    if (merchant_controller->is_open())
        merchant_controller->render();

    {
        int px, py;
        if (world_renderer.sprites().get_movable_position(px, py)) {
            audio_manager.set_player_position(px, py);
        }
    }

    renderer.Present();
}

void GameController::apply_server_event(const ServerEvent& ev) { event_handler_.apply(ev); }

void GameController::load_game_assets() {
    world_renderer.load_assets(config.tilemap, config.sprites, config.skins, config.item_sprites,
                               config.ground_item, config.damage_overlay, config.spell_sheets,
                               config.walk_anim_timeout_ms);
}

void GameController::apply_movement_visual(Direction dir, bool advance_frame) {
    world_renderer.sprites().set_movable_src_y(move_config.body_src_y_for(dir));
    world_renderer.sprites().set_anchor_src_y(move_config.head_src_y_for(dir));
    if (advance_frame) {
        world_renderer.sprites().advance_movable_src_x(move_config.walk_src_step,
                                                       move_config.walk_src_frames_for(dir));
    }
}

bool GameController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        return false;
    }

    if (chat_input.consume_event(event)) {
        flush_pending_chat();
        if (chat_expanded_ && !chat_input.is_focused()) {
            chat_expanded_ = false;
            ui_renderer.set_chat_expanded(false);
        }
        return true;
    }

    if (event.type == SDL_MOUSEWHEEL) {
        if (merchant_controller->is_open()) {
            merchant_controller->handle_scroll(event.wheel.y);
            return true;
        }
        int max_scroll = static_cast<int>(chat_history.get_messages().size()) - 1;
        chat_scroll = std::clamp(chat_scroll + event.wheel.y, 0, std::max(0, max_scroll));
        chat_history.consume_new_message();
        return true;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        return handle_mouse_button(event);
    }

    if (event.type == SDL_MOUSEMOTION) {
        return handle_mouse_motion(event);
    }

    if (event.type == SDL_KEYDOWN) {
        return handle_keydown(event);
    }

    return true;
}

bool GameController::handle_mouse_button(const SDL_Event& event) {
    if (merchant_controller->is_open())
        return merchant_controller->handle_mouse_button(event);

    if (try_handle_ui_click(event))
        return true;

    if (try_handle_inventory_click(event))
        return true;

    return try_handle_world_click(event);
}

bool GameController::try_handle_ui_click(const SDL_Event& event) {
    const bool left = event.button.button == SDL_BUTTON_LEFT;

    if (left && ui_renderer.is_audio_hit(event.button.x, event.button.y)) {
        audio_manager.set_muted(!audio_manager.is_muted());
        return true;
    }

    if (left && ui_renderer.is_expand_hit(event.button.x, event.button.y)) {
        chat_expanded_ = !chat_expanded_;
        ui_renderer.set_chat_expanded(chat_expanded_);
        if (chat_expanded_ && !chat_input.is_focused())
            chat_input.set_focus(true);
        return true;
    }

    const bool was_focused = chat_input.is_focused();
    chat_input.set_focus(ui_renderer.is_chat_input_hit(event.button.x, event.button.y));
    if (was_focused && !chat_input.is_focused() && chat_expanded_) {
        chat_expanded_ = false;
        ui_renderer.set_chat_expanded(false);
    }

    return chat_input.is_focused();
}

bool GameController::try_handle_inventory_click(const SDL_Event& event) {
    if (event.button.button != SDL_BUTTON_LEFT)
        return false;

    const int potion = ui_renderer.get_hovered_potion();
    if (potion > 0) {
        const int slot = (potion == 1) ? ui_renderer.get_first_hp_potion_slot() :
                                         ui_renderer.get_first_mana_potion_slot();
        if (slot >= 0)
            command_queue.push(EquipItemCmd{static_cast<uint8_t>(slot)});
        return true;
    }

    if (!ui_renderer.is_hovering_occupied())
        return false;

    const int idx = ui_renderer.get_hovered_inv_slot();
    if (idx >= 0) {
        command_queue.push(EquipItemCmd{static_cast<uint8_t>(idx)});
        return true;
    }
    const int eq = ui_renderer.get_hovered_equip_slot();
    if (eq >= 0) {
        command_queue.push(UnequipItemCmd{static_cast<EquipSlot>(eq)});
        return true;
    }
    return false;
}

bool GameController::try_handle_world_click(const SDL_Event& event) {
    int world_x = 0, world_y = 0;
    if (!world_renderer.screen_to_world(event.button.x, event.button.y, world_x, world_y))
        return true;

    const bool left = event.button.button == SDL_BUTTON_LEFT;
    const bool right = event.button.button == SDL_BUTTON_RIGHT;

    if (!player_is_ghost) {
        uint16_t entity_id = 0;
        if (world_renderer.sprites().hit_test_entity(world_x, world_y, entity_id)) {
            if (right)
                command_queue.push(CastSpellCmd{entity_id});
            else
                command_queue.push(AttackCmd{entity_id});
            return true;
        }

        if (right) {
            int sx, sy;
            if (world_renderer.sprites().get_movable_position(sx, sy)) {
                SDL2pp::Rect self_rect(sx, sy, world_renderer.sprites().movable_w(),
                                       world_renderer.sprites().movable_h());
                if (point_in_rect(world_x, world_y, self_rect)) {
                    command_queue.push(CastSpellCmd{player_stats.player_id});
                    return true;
                }
            }
        }

        if (left) {
            std::string prop_name;
            if (world_renderer.hit_test_prop(world_x, world_y, prop_name)) {
                interact_with_prop(prop_name, world_x, world_y);
                return true;
            }
        }
    }

    if (left)
        move_controller.set_move_target(world_x, world_y);
    return true;
}

bool GameController::handle_mouse_motion(const SDL_Event& event) {
    mouse_x = event.motion.x;
    mouse_y = event.motion.y;

    if (merchant_controller->is_open()) {
        merchant_controller->handle_mouse_motion(mouse_x, mouse_y);
        SDL_SetCursor(merchant_controller->is_any_button_hovered() ? hand_cursor : arrow_cursor);
        return true;
    }

    ui_renderer.set_audio_button_hovered(mouse_x, mouse_y);
    ui_renderer.set_expand_button_hovered(mouse_x, mouse_y);
    ui_renderer.set_hover(mouse_x, mouse_y, player_stats.inventory, player_stats.equipped);
    ui_renderer.update_potion_button_hover(mouse_x, mouse_y, player_stats.inventory);
    if (ui_renderer.is_audio_hit(mouse_x, mouse_y) || ui_renderer.is_expand_hit(mouse_x, mouse_y) ||
        ui_renderer.is_hovering_occupied() || ui_renderer.get_hovered_potion() > 0) {
        SDL_SetCursor(hand_cursor);
        return true;
    }

    int world_x = 0;
    int world_y = 0;
    if (world_renderer.screen_to_world(event.motion.x, event.motion.y, world_x, world_y)) {
        uint16_t entity_id = 0;
        std::string prop_name;
        bool show_hand = world_renderer.sprites().hit_test_entity(world_x, world_y, entity_id);
        if (!show_hand && world_renderer.hit_test_prop(world_x, world_y, prop_name))
            show_hand = is_clickable_prop(prop_name);
        SDL_SetCursor(show_hand ? hand_cursor : arrow_cursor);
    } else {
        SDL_SetCursor(arrow_cursor);
    }
    return true;
}

bool GameController::handle_keydown(const SDL_Event& event) {
    const uint32_t now = SDL_GetTicks();
    const bool ctrl = event.key.keysym.mod & KMOD_CTRL;

    if (merchant_controller->handle_keydown(event))
        return true;

    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            return false;
        case SDLK_LEFT:
            move_controller.cancel_move_target();
            apply_movement_visual(Direction::WEST,
                                  move_controller.move_direction(Direction::WEST, now).has_value());
            break;
        case SDLK_RIGHT:
            move_controller.cancel_move_target();
            apply_movement_visual(Direction::EAST,
                                  move_controller.move_direction(Direction::EAST, now).has_value());
            break;
        case SDLK_UP:
            move_controller.cancel_move_target();
            apply_movement_visual(
                    Direction::NORTH,
                    move_controller.move_direction(Direction::NORTH, now).has_value());
            break;
        case SDLK_DOWN:
            move_controller.cancel_move_target();
            apply_movement_visual(
                    Direction::SOUTH,
                    move_controller.move_direction(Direction::SOUTH, now).has_value());
            break;
        case SDLK_h:
            if (!ctrl)
                world_renderer.set_show_hitboxes(!world_renderer.get_show_hitboxes());
            break;
        default:
            break;
    }

    if (ctrl) {
        auto it = cheat_commands_.find(event.key.keysym.sym);
        if (it != cheat_commands_.end())
            command_queue.push(it->second);
        return true;
    }

    return true;
}

void GameController::interact_with_prop(const std::string& prop_name, int world_x, int world_y) {
    auto it = config.get_interactable_props().find(prop_name);
    if (it != config.get_interactable_props().end()) {
        const PropConfig& prop = it->second;
        if (!prop.sfx.empty())
            audio_manager.play_sfx_at(prop.sfx, world_x, world_y);
        if (!prop.dialog.empty())
            chat_history.add_message(ChatMsgType::SYSTEM, "", prop.dialog);
        if (prop.opens_merchant)
            merchant_controller->open(prop.merchant_sell_enabled);
        return;
    }
    if (is_transition_prop(prop_name)) {
        command_queue.push(ChangeMapCmd{prop_name});
    }
}

bool GameController::is_clickable_prop(const std::string& prop_name) const {
    return config.get_interactable_props().count(prop_name) > 0 || is_transition_prop(prop_name);
}

bool GameController::is_transition_prop(const std::string& prop_name) const {
    auto map_it = config.tilemap_configs.find(current_map_name);
    if (map_it == config.tilemap_configs.end())
        return false;
    auto prop_it = map_it->second.props.find(prop_name);
    if (prop_it == map_it->second.props.end())
        return false;
    return !prop_it->second.transition_map.empty();
}

void GameController::flush_pending_chat() {
    if (!chat_input.has_pending_message())
        return;
    std::string text = chat_input.pop_pending_message();

    auto cmd = chat_parser_.parse(text);
    if (cmd) {
        command_queue.push(std::move(*cmd));
    } else {
        command_queue.push(SendChatMsgCmd{std::move(text)});
    }
}
