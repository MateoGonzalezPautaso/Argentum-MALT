#include "game_controller.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "../../common/visit.h"
#include "../render/geometry.h"

namespace {
const std::unordered_map<ItemType, std::string> weapon_sounds = {
        {ItemType::SWORD, "sword"},         {ItemType::AXE, "axe"},
        {ItemType::HAMMER, "hammer"},       {ItemType::SIMPLE_BOW, "bow"},
        {ItemType::COMPOSITE_BOW, "bow"},   {ItemType::ASH_STAFF, "spell"},
        {ItemType::KNOTTED_STAFF, "spell"}, {ItemType::STUDDED_STAFF, "spell"},
        {ItemType::ELVEN_FLUTE, "flute"},
};
}

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
        arrow_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW)) {
    world_renderer.set_direction_src_y(config.dir_src_y_down, config.dir_src_y_up,
                                       config.dir_src_y_left, config.dir_src_y_right);
    merchant_controller = std::make_unique<MerchantController>(renderer, config.ui,
                                                               command_queue, player_stats);
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
    ui_renderer.render_frame_background();
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
    ui_renderer.set_hover(mouse_x, mouse_y, player_stats.inventory, player_stats.equipped);
    ui_renderer.update_potion_button_hover(mouse_x, mouse_y, player_stats.inventory);
    ui_renderer.render_inventory(player_stats.inventory);
    ui_renderer.render_equipped(player_stats.equipped);
    ui_renderer.render_stat_tooltips(mouse_x, mouse_y);
    if (merchant_controller->is_open())
        merchant_controller->render();

    {
        int px, py;
        if (world_renderer.get_movable_position(px, py)) {
            audio_manager.set_player_position(px, py);
        }
    }

    renderer.Present();
}

void GameController::apply_server_event(const ServerEvent& ev) {
    std::visit(
            overloaded{
                    [this](const EntityMoveEvent& e) { handle_entity_move(e); },
                    [this](const EntitySpawnEvent& e) { handle_entity_spawn(e); },
                    [this](const LoginOkEvent& e) { handle_login_ok(e); },
                    [this](const EntityDespawnEvent& e) { handle_entity_despawn(e); },
                    [this](const DamageReceivedEvent& e) {
                        uint16_t src = (e.target_id == player_stats.player_id)
                                               ? e.attacker_id
                                               : e.target_id;
                        play_spatial_sfx("hit", src);
                        handle_damage_received(e);
                    },
                    [this](const DamageDealtEvent& e) {
                        auto weapon = player_stats.equipped[0].item_type;
                        auto it = weapon_sounds.find(weapon);
                        if (it != weapon_sounds.end()) {
                            play_spatial_sfx(it->second, e.target_id);
                        }
                    },
                    [this](const AttackDodgedEvent& e) { handle_attack_dodged(e); },
                    [this](const ChatMsgEvent& e) { handle_chat_msg(e); },
                    [this](const EntityDiedEvent& e) { handle_entity_died(e); },
                    [this](const MeditationStartEvent&) { audio_manager.play_sfx("meditate"); },
                    [this](const PlayerRespawnedEvent& e) { handle_player_respawned(e); },
                    [this](const ClanNotificationEvent& e) { handle_clan_notification(e); },
                    [this](const ClanUpdateEvent& e) { handle_clan_update(e); },
                    [this](const MapTransitionEvent& e) { handle_map_transition(e); },
                    [this](const HealReceivedEvent& e) { handle_heal_received(e); },
                    [this](const InventoryUpdateEvent& e) { handle_inventory_update(e); },
                    [this](const EquipUpdateEvent& e) { handle_equip_update(e); },
                    [this](const PlayerStatsEvent& e) { handle_player_stats(e); },
                    [this](const SpellEffectEvent& e) {
                        int wx, wy;
                        if (e.target_id == player_stats.player_id) {
                            world_renderer.get_movable_position(wx, wy);
                            wx += world_renderer.movable_w() / 2;
                            wy += world_renderer.movable_h() / 2;
                        } else if (!world_renderer.get_entity_world_position(e.target_id, wx, wy)) {
                            return;
                        }
                        world_renderer.trigger_spell_effect(e.effect_type, wx, wy);
                    },
                    [this](const GoldUpdateEvent& e) { player_stats.gold = e.gold; },
                    [this](const NpcItemListEvent& e) {
                        merchant_controller->on_item_list(e);
                        for (const auto& item : e.items)
                            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                                     item.item_name + ": " +
                                                             std::to_string(item.price) +
                                                             " de oro");
                    },
                    [this](const BankUpdateEvent& e) { handle_bank_update(e); },
                    [this](const ItemDroppedEvent& e) {
                        world_renderer.spawn_ground_item(e.pos.x, e.pos.y, e.item_type,
                                                         e.item_name);
                    },
                    [this](const ItemPickedEvent& e) {
                        world_renderer.despawn_ground_item(e.pos.x, e.pos.y, e.item_name);
                    },
                    [](const auto&) {},
            },
            ev);
}

void GameController::handle_entity_move(const EntityMoveEvent& e) {
    if (e.entity_id == player_stats.player_id) {
        move_controller.set_position(e.entity_pos.x, e.entity_pos.y);
        world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
        return;
    }
    world_renderer.move_entity(e.entity_id, e.entity_pos.x, e.entity_pos.y);
    world_renderer.note_entity_moved(e.entity_id);
    world_renderer.set_entity_src_y(e.entity_id, move_config.body_src_y_for(e.entity_dir),
                                    move_config.head_src_y_for(e.entity_dir));
    world_renderer.step_entity_src_x(e.entity_id, move_config.walk_src_step,
                                     move_config.walk_src_frames_for(e.entity_dir));
}

void GameController::handle_entity_spawn(const EntitySpawnEvent& e) {
    if (e.entity_id == player_stats.player_id) {
        move_controller.set_position(e.entity_pos.x, e.entity_pos.y);
        world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
        return;
    }
    world_renderer.spawn_entity(e.entity_id, e.entity_pos.x, e.entity_pos.y, e.entity_name,
                                e.entity_race, e.entity_class, e.sprite_id);
    world_renderer.set_entity_src_y(e.entity_id, move_config.body_src_y_for(e.entity_dir),
                                    move_config.head_src_y_for(e.entity_dir));

    if (e.entity_type != EntityType::NPC) {
        static constexpr uint8_t overlay_slots[] = {
                static_cast<uint8_t>(EquipSlot::WEAPON),
                static_cast<uint8_t>(EquipSlot::HELMET),
                static_cast<uint8_t>(EquipSlot::SHIELD),
        };
        const ItemType equipped_types[EQUIP_SLOT_COUNT] = {e.weapon_type, e.armor_type, e.helmet_type,
                                                           e.shield_type};
        for (uint8_t slot: overlay_slots) {
            ItemType type = equipped_types[slot];
            if (type == ItemType::NONE) {
                world_renderer.clear_entity_equipment_overlay(e.entity_id, slot);
                continue;
            }
            auto it = config.equip_overlays.find(static_cast<uint8_t>(type));
            if (it != config.equip_overlays.end()) {
                world_renderer.update_entity_equipment_overlay(e.entity_id, slot, it->second.path,
                                                               it->second.offset_y,
                                                               it->second.static_frame);
            } else {
                world_renderer.clear_entity_equipment_overlay(e.entity_id, slot);
            }
        }

        ItemType armor_type = e.armor_type;
        if (armor_type == ItemType::NONE) {
            world_renderer.reset_entity_body_sprite(e.entity_id);
        } else {
            auto it = config.equip_overlays.find(static_cast<uint8_t>(armor_type));
            if (it != config.equip_overlays.end()) {
                world_renderer.set_entity_body_sprite(e.entity_id, it->second.path);
            } else {
                world_renderer.reset_entity_body_sprite(e.entity_id);
            }
        }
    }
}

void GameController::load_game_assets() {
    world_renderer.load_assets(config.tilemap, config.sprites, config.skins, config.item_sprites,
                               config.ground_item);
}

void GameController::handle_login_ok(const LoginOkEvent& e) {
    player_stats.player_id = e.player_id;
    player_stats.username = e.username;
    player_stats.race = e.race;
    player_stats.player_class = e.player_class;
    player_stats.level = e.level;
    player_stats.experience = e.experience;
    player_stats.exp_to_next = e.exp_to_next;
    player_stats.hp_current = e.hp_current;
    player_stats.hp_max = e.hp_max;
    player_stats.mana_current = e.mana_current;
    player_stats.mana_max = e.mana_max;
    player_stats.gold = e.gold;
    player_stats.pos = e.pos;
    player_is_ghost = (e.hp_current == 0);
    move_controller.set_position(e.pos.x, e.pos.y);
    world_renderer.set_movable_position(e.pos.x, e.pos.y);
    world_renderer.set_local_player_info(e.race, e.player_class);
    if (player_is_ghost)
        world_renderer.set_movable_alpha(128);
}

void GameController::handle_entity_despawn(const EntityDespawnEvent& e) {
    world_renderer.despawn_entity(e.entity_id);
}

void GameController::handle_damage_received(const DamageReceivedEvent& e) {
    if (e.target_id == player_stats.player_id) {
        player_stats.hp_current = e.hp_current;
        player_stats.hp_max = e.hp_max;
    }
    int wx, wy;
    if (e.target_id == player_stats.player_id) {
        world_renderer.get_movable_position(wx, wy);
        wx += world_renderer.movable_w() / 2;
        wy += world_renderer.movable_h() / 2;
    } else if (!world_renderer.get_entity_world_position(e.target_id, wx, wy)) {
        return;
    }
    world_renderer.trigger_damage_overlay_at(wx, wy);
}

void GameController::interact_with_prop(const std::string& prop_name, int world_x, int world_y) {
    if (prop_name == "sacerdote") {
        audio_manager.play_sfx_at("priest", world_x, world_y);
        chat_history.add_message(ChatMsgType::SYSTEM, "", "Sacerdote: ¡SHALOM!");
        merchant_controller->open(false);
    } else if (prop_name == "comerciante") {
        audio_manager.play_sfx_at("merchant", world_x, world_y);
        chat_history.add_message(ChatMsgType::SYSTEM, "",
                                 "Comerciante: Pasa, todo lo que ves esta en venta.");
        merchant_controller->open(true);
    } else if (prop_name == "banquero") {
        audio_manager.play_sfx_at("banker", world_x, world_y);
        chat_history.add_message(ChatMsgType::SYSTEM, "",
                                 "Banquero: El que deposita dolares, recibira dolares...");
    } else if (prop_name == "sanadora") {
        audio_manager.play_sfx_at("healer", world_x, world_y);
        chat_history.add_message(ChatMsgType::SYSTEM, "", "Sanadora: Dejame ver esa herida.");
    } else if (is_transition_prop(prop_name)) {
        command_queue.push(ChangeMapCmd{prop_name});
    }
}

bool GameController::is_clickable_prop(const std::string& prop_name) const {
    return prop_name == "sacerdote" || prop_name == "comerciante" || prop_name == "banquero" ||
           prop_name == "sanadora" || is_transition_prop(prop_name);
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

void GameController::handle_attack_dodged(const AttackDodgedEvent& e) {
    if (e.player_id == player_stats.player_id)
        chat_history.add_message(ChatMsgType::SYSTEM, "", "Esquivaste el ataque");
    else
        chat_history.add_message(ChatMsgType::SYSTEM, "", "El ataque fue esquivado");
}

void GameController::handle_chat_msg(const ChatMsgEvent& e) {
    chat_history.add_message(e.type, e.sender_name, e.message);
}

void GameController::handle_clan_notification(const ClanNotificationEvent& e) {
    switch (e.type) {
        case ClanNotifType::MEMBER_ONLINE:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " esta en linea");
            break;
        case ClanNotifType::MEMBER_OFFLINE:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " se desconecto");
            break;
        case ClanNotifType::MEMBER_ATTACKED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " esta siendo atacado!");
            break;
        case ClanNotifType::JOIN_REQUEST:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " quiere unirse al clan");
            break;
        case ClanNotifType::JOIN_ACCEPTED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] Has sido aceptado en " + e.clan_name);
            break;
        case ClanNotifType::JOIN_REJECTED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] Has sido rechazado de " + e.clan_name);
            break;
        case ClanNotifType::KICKED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] Has sido expulsado de " + e.clan_name);
            break;
    }
}

void GameController::handle_clan_update(const ClanUpdateEvent& e) {
    std::string msg = "--- Clan: " + e.clan_name + " ---";
    for (const auto& m: e.members) {
        msg += "\n  " + m.username;
        if (m.is_founder)
            msg += " (fundador)";
        msg += m.is_online ? " [En linea]" : " [Desconectado]";
    }
    chat_history.add_message(ChatMsgType::SYSTEM, "", msg);
}

void GameController::handle_inventory_update(const InventoryUpdateEvent& e) {
    player_stats.inventory = e.slots;
}

void GameController::handle_equip_update(const EquipUpdateEvent& e) {
    static constexpr uint8_t overlay_slots[] = {
            static_cast<uint8_t>(EquipSlot::WEAPON),
            static_cast<uint8_t>(EquipSlot::HELMET),
            static_cast<uint8_t>(EquipSlot::SHIELD),
    };

    InventorySlot equipped[EQUIP_SLOT_COUNT] = {e.weapon, e.armor, e.helmet, e.shield};
    const bool is_local = (e.entity_id == player_stats.player_id);

    if (is_local) {
        player_stats.equipped[0] = e.weapon;
        player_stats.equipped[1] = e.armor;
        player_stats.equipped[2] = e.helmet;
        player_stats.equipped[3] = e.shield;
    }

    for (uint8_t slot: overlay_slots) {
        ItemType type = equipped[slot].item_type;
        if (type == ItemType::NONE) {
            if (is_local)
                world_renderer.clear_equipment_overlay(slot);
            else
                world_renderer.clear_entity_equipment_overlay(e.entity_id, slot);
        } else {
            auto it = config.equip_overlays.find(static_cast<uint8_t>(type));
            if (it != config.equip_overlays.end()) {
                if (is_local) {
                    world_renderer.update_equipment_overlay(
                            slot, it->second.path, it->second.offset_y, it->second.static_frame);
                } else {
                    world_renderer.update_entity_equipment_overlay(
                            e.entity_id, slot, it->second.path, it->second.offset_y,
                            it->second.static_frame);
                }
            } else {
                if (is_local)
                    world_renderer.clear_equipment_overlay(slot);
                else
                    world_renderer.clear_entity_equipment_overlay(e.entity_id, slot);
            }
        }
    }

    ItemType armor_type = equipped[static_cast<uint8_t>(EquipSlot::ARMOR)].item_type;
    if (armor_type == ItemType::NONE) {
        if (is_local)
            world_renderer.reset_body_sprite();
        else
            world_renderer.reset_entity_body_sprite(e.entity_id);
    } else {
        auto it = config.equip_overlays.find(static_cast<uint8_t>(armor_type));
        if (it != config.equip_overlays.end()) {
            if (is_local)
                world_renderer.set_body_sprite(it->second.path);
            else
                world_renderer.set_entity_body_sprite(e.entity_id, it->second.path);
        } else {
            if (is_local)
                world_renderer.reset_body_sprite();
            else
                world_renderer.reset_entity_body_sprite(e.entity_id);
        }
    }
}

void GameController::handle_entity_died(const EntityDiedEvent& e) {
    world_renderer.set_entity_alpha(e.entity_id, 128);
    if (e.entity_id != player_stats.player_id) {
        return;
    }
    audio_manager.play_sfx("death");
    player_is_ghost = true;
    world_renderer.set_movable_alpha(128);
}

void GameController::handle_player_respawned(const PlayerRespawnedEvent& e) {
    world_renderer.set_entity_alpha(e.entity_id, 255);
    if (e.entity_id != player_stats.player_id) {
        return;
    }
    player_is_ghost = false;
    player_stats.hp_current = e.hp_current;
    player_stats.hp_max = e.hp_max;
    world_renderer.set_movable_alpha(255);
}

void GameController::handle_heal_received(const HealReceivedEvent& e) {
    if (e.player_id != player_stats.player_id)
        return;
    player_stats.hp_current = e.hp_current;
    player_stats.mana_current = e.mana_current;
}

void GameController::apply_movement_visual(Direction dir, bool advance_frame) {
    world_renderer.set_movable_src_y(move_config.body_src_y_for(dir));
    world_renderer.set_anchor_src_y(move_config.head_src_y_for(dir));
    if (advance_frame) {
        world_renderer.step_movable_src_x(move_config.walk_src_step,
                                          move_config.walk_src_frames_for(dir));
    }
}

bool GameController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        return false;
    }

    if (chat_input.consume_event(event)) {
        flush_pending_chat();
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

    chat_input.set_focus(ui_renderer.is_chat_input_hit(event.button.x, event.button.y));

    if (chat_input.is_focused()) {
        return true;
    }

    if (event.button.button == SDL_BUTTON_LEFT) {
        int potion = ui_renderer.get_hovered_potion();
        if (potion > 0) {
            int slot = (potion == 1) ? ui_renderer.get_first_hp_potion_slot()
                                     : ui_renderer.get_first_mana_potion_slot();
            if (slot >= 0)
                command_queue.push(EquipItemCmd{static_cast<uint8_t>(slot)});
            return true;
        }

        if (ui_renderer.is_hovering_occupied()) {
            int idx = ui_renderer.get_hovered_inv_slot();
            if (idx >= 0) {
                command_queue.push(EquipItemCmd{static_cast<uint8_t>(idx)});
                return true;
            }
            int eq = ui_renderer.get_hovered_equip_slot();
            if (eq >= 0) {
                command_queue.push(UnequipItemCmd{static_cast<EquipSlot>(eq)});
                return true;
            }
        }
    }

    int world_x = 0;
    int world_y = 0;
    if (!world_renderer.screen_to_world(event.button.x, event.button.y, world_x, world_y)) {
        return true;
    }

    if (!player_is_ghost) {
        uint16_t entity_id = 0;
        if (world_renderer.hit_test_entity(world_x, world_y, entity_id)) {
            if (event.button.button == SDL_BUTTON_RIGHT) {
                command_queue.push(CastSpellCmd{entity_id});
            } else {
                command_queue.push(AttackCmd{entity_id});
            }
            return true;
        }

        if (event.button.button == SDL_BUTTON_RIGHT) {
            int sx, sy;
            if (world_renderer.get_movable_position(sx, sy)) {
                SDL2pp::Rect self_rect(sx, sy, world_renderer.movable_w(),
                                       world_renderer.movable_h());
                if (point_in_rect(world_x, world_y, self_rect)) {
                    command_queue.push(CastSpellCmd{player_stats.player_id});
                    return true;
                }
            }
        }

        if (event.button.button == SDL_BUTTON_LEFT) {
            std::string prop_name;
            if (world_renderer.hit_test_prop(world_x, world_y, prop_name)) {
                interact_with_prop(prop_name, world_x, world_y);
                return true;
            }
        }
    }

    if (event.button.button == SDL_BUTTON_LEFT) {
        move_controller.set_move_target(world_x, world_y);
    }
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

    ui_renderer.set_hover(mouse_x, mouse_y, player_stats.inventory, player_stats.equipped);
    ui_renderer.update_potion_button_hover(mouse_x, mouse_y, player_stats.inventory);
    if (ui_renderer.is_hovering_occupied() || ui_renderer.get_hovered_potion() > 0) {
        SDL_SetCursor(hand_cursor);
        return true;
    }

    int world_x = 0;
    int world_y = 0;
    if (world_renderer.screen_to_world(event.motion.x, event.motion.y, world_x, world_y)) {
        uint16_t entity_id = 0;
        std::string prop_name;
        bool show_hand = world_renderer.hit_test_entity(world_x, world_y, entity_id);
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
            if (ctrl)
                command_queue.push(CheatInfiniteHpCmd{});
            else
                world_renderer.set_show_hitboxes(!world_renderer.get_show_hitboxes());
            break;
        case SDLK_i:
            if (ctrl)
                command_queue.push(CheatFillInventoryCmd{});
            break;
        case SDLK_m:
            if (ctrl)
                command_queue.push(CheatInfiniteManaCmd{});
            break;
        case SDLK_k:
            if (ctrl)
                command_queue.push(CheatDieCmd{});
            break;
        case SDLK_l:
            if (ctrl)
                command_queue.push(CheatClearInventoryCmd{});
            break;
        case SDLK_v:
            if (ctrl)
                command_queue.push(CheatLevelUpCmd{});
            break;
        case SDLK_b:
            if (ctrl)
                command_queue.push(CheatLevelDownCmd{});
            break;
        case SDLK_g:
            if (ctrl)
                command_queue.push(CheatAddGoldCmd{});
            break;
        case SDLK_0:
            if (ctrl)
                command_queue.push(CheatResetGoldCmd{});
            break;
        case SDLK_9:
            if (ctrl)
                command_queue.push(CheatResetManaCmd{});
            break;
        case SDLK_f:
            if (ctrl)
                command_queue.push(CheatVelocityCmd{});
            break;
        case SDLK_r:
            if (ctrl)
                command_queue.push(CheatReviveCmd{});
            break;
        default:
            break;
    }

    return true;
}

void GameController::handle_map_transition(const MapTransitionEvent& e) {
    auto it = config.tilemap_configs.find(e.map_name);
    if (it == config.tilemap_configs.end())
        return;

    current_map_name = e.map_name;
    world_renderer.clear_entities();
    world_renderer.clear_ground_items();
    world_renderer.load_map(it->second);
    world_renderer.set_movable_position(e.pos_x, e.pos_y);
    move_controller.set_position(e.pos_x, e.pos_y);
    player_stats.pos = {e.pos_x, e.pos_y};
}

void GameController::handle_bank_update(const BankUpdateEvent& e) {
    chat_history.add_message(ChatMsgType::SYSTEM, "", "Banco - Oro: " + std::to_string(e.gold));
    for (const auto& slot : e.slots) {
        if (slot.item_type != ItemType::NONE)
            chat_history.add_message(ChatMsgType::SYSTEM, "", "Banco - " + slot.item_name);
    }
}

void GameController::handle_player_stats(const PlayerStatsEvent& e) {
    if (e.level > player_stats.level) {
        audio_manager.play_sfx("level_up");
    }
    player_stats.level = e.level;
    player_stats.experience = e.experience;
    player_stats.exp_to_next = e.exp_to_next;
    player_stats.hp_current = e.hp_current;
    player_stats.hp_max = e.hp_max;
    player_stats.mana_current = e.mana_current;
    player_stats.mana_max = e.mana_max;
    player_stats.crit_chance = e.crit_chance;
    player_stats.damage_min = e.damage_min;
    player_stats.damage_max = e.damage_max;
    player_stats.defense_min = e.defense_min;
    player_stats.defense_max = e.defense_max;
    player_stats.dodge_chance = e.dodge_chance;
    player_stats.strength = e.strength;
    player_stats.agility = e.agility;
}

void GameController::play_spatial_sfx(const std::string& name, uint16_t entity_id) {
    int sx, sy;
    if (world_renderer.get_entity_world_position(entity_id, sx, sy)) {
        audio_manager.play_sfx_at(name, sx, sy);
    } else {
        audio_manager.play_sfx(name);
    }
}

void GameController::flush_pending_chat() {
    if (!chat_input.has_pending_message())
        return;
    std::string text = chat_input.pop_pending_message();

    if (text == "/listar") {
        command_queue.push(NpcListCmd{});
    } else if (text.rfind("/comprar ", 0) == 0) {
        command_queue.push(NpcBuyCmd{text.substr(9)});
    } else if (text.rfind("/vender ", 0) == 0) {
        command_queue.push(NpcSellCmd{text.substr(8)});
    } else if (text == "/meditar") {
        command_queue.push(MeditateCmd{});
    } else if (text == "/resucitar") {
        command_queue.push(ResurrectCmd{});
    } else if (text == "/curar") {
        command_queue.push(NpcHealCmd{});
    } else if (text == "/tomar") {
        command_queue.push(PickupItemCmd{""});
    } else if (text.rfind("/tomar ", 0) == 0) {
        command_queue.push(PickupItemCmd{text.substr(7)});
    } else if (text.rfind("/tirar ", 0) == 0) {
        command_queue.push(DropItemCmd{text.substr(7)});
    } else if (text.rfind("/depositar oro ", 0) == 0) {
        try {
            uint32_t amount = static_cast<uint32_t>(std::stoul(text.substr(15)));
            command_queue.push(BankDepositCmd{true, "", amount});
        } catch (...) {
            chat_history.add_message(ChatMsgType::SYSTEM, "", "Uso: /depositar oro <cantidad>");
        }
    } else if (text.rfind("/depositar ", 0) == 0) {
        command_queue.push(BankDepositCmd{false, text.substr(11), 0});
    } else if (text.rfind("/retirar oro ", 0) == 0) {
        try {
            uint32_t amount = static_cast<uint32_t>(std::stoul(text.substr(13)));
            command_queue.push(BankWithdrawCmd{true, "", amount});
        } catch (...) {
            chat_history.add_message(ChatMsgType::SYSTEM, "", "Uso: /retirar oro <cantidad>");
        }
    } else if (text.rfind("/retirar ", 0) == 0) {
        command_queue.push(BankWithdrawCmd{false, text.substr(9), 0});
    } else if (text.rfind("/equipar ", 0) == 0) {
        try {
            int idx = std::stoi(text.substr(9));
            command_queue.push(EquipItemCmd{static_cast<uint8_t>(idx)});
        } catch (...) {
            chat_history.add_message(ChatMsgType::SYSTEM, "", "Uso: /equipar <slot>");
        }
    } else if (text.rfind("/desequipar ", 0) == 0) {
        std::string slot = text.substr(12);
        if (slot == "weapon")
            command_queue.push(UnequipItemCmd{EquipSlot::WEAPON});
        else if (slot == "armor")
            command_queue.push(UnequipItemCmd{EquipSlot::ARMOR});
        else if (slot == "helmet")
            command_queue.push(UnequipItemCmd{EquipSlot::HELMET});
        else if (slot == "shield")
            command_queue.push(UnequipItemCmd{EquipSlot::SHIELD});
    } else if (text.rfind("/fundar-clan ", 0) == 0) {
        command_queue.push(ClanFoundCmd{text.substr(13)});
    } else if (text.rfind("/unirse ", 0) == 0) {
        command_queue.push(ClanJoinRequestCmd{text.substr(8)});
    } else if (text == "/revisar-clan") {
        command_queue.push(ClanReviewCmd{});
    } else if (text.rfind("/clan-aceptar ", 0) == 0) {
        command_queue.push(ClanAcceptCmd{text.substr(14)});
    } else if (text.rfind("/clan-rechazar ", 0) == 0) {
        command_queue.push(ClanRejectCmd{text.substr(15)});
    } else if (text.rfind("/clan-unban ", 0) == 0) {
        command_queue.push(ClanUnbanCmd{text.substr(12)});
    } else if (text.rfind("/clan-ban ", 0) == 0) {
        command_queue.push(ClanBanCmd{text.substr(10)});
    } else if (text.rfind("/clan-kick ", 0) == 0) {
        command_queue.push(ClanKickCmd{text.substr(11)});
    } else if (text == "/dejar-clan") {
        command_queue.push(ClanLeaveCmd{});
    } else {
        command_queue.push(SendChatMsgCmd{std::move(text)});
    }
}
