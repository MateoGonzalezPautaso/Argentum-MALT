#include "server_event_handler.h"

#include <string>
#include <unordered_map>
#include <variant>

#include "../../common/visit.h"
#include "../ui/controllers/merchant_controller.h"

namespace {
const std::unordered_map<ItemType, std::string> weapon_sounds = {
        {ItemType::SWORD, "sword"},         {ItemType::AXE, "axe"},
        {ItemType::HAMMER, "hammer"},       {ItemType::SIMPLE_BOW, "bow"},
        {ItemType::COMPOSITE_BOW, "bow"},   {ItemType::ASH_STAFF, "spell"},
        {ItemType::KNOTTED_STAFF, "spell"}, {ItemType::STUDDED_STAFF, "spell"},
        {ItemType::ELVEN_FLUTE, "flute"},
};
}

ServerEventHandler::ServerEventHandler(PlayerStats& player_stats, WorldRenderer& world_renderer,
                                       AudioManager& audio_manager, ChatHistory& chat_history,
                                       MoveController& move_controller,
                                       const ClientConfig& config, const MoveConfig& move_config,
                                       MerchantController& merchant_controller,
                                       bool& player_is_ghost, std::string& current_map_name):
        player_stats_(player_stats),
        world_renderer_(world_renderer),
        audio_manager_(audio_manager),
        chat_history_(chat_history),
        move_controller_(move_controller),
        config_(config),
        move_config_(move_config),
        merchant_controller_(merchant_controller),
        player_is_ghost_(player_is_ghost),
        current_map_name_(current_map_name) {}

void ServerEventHandler::apply(const ServerEvent& ev) {
    std::visit(
            overloaded{
                    [this](const EntityMoveEvent& e) { handle_entity_move(e); },
                    [this](const EntitySpawnEvent& e) { handle_entity_spawn(e); },
                    [this](const LoginOkEvent& e) { handle_login_ok(e); },
                    [this](const EntityDespawnEvent& e) { handle_entity_despawn(e); },
                    [this](const DamageReceivedEvent& e) {
                        uint16_t src = (e.target_id == player_stats_.player_id)
                                               ? e.attacker_id
                                               : e.target_id;
                        play_spatial_sfx("hit", src);
                        handle_damage_received(e);
                    },
                    [this](const DamageDealtEvent& e) {
                        auto weapon = player_stats_.equipped[0].item_type;
                        auto it = weapon_sounds.find(weapon);
                        if (it != weapon_sounds.end()) {
                            play_spatial_sfx(it->second, e.target_id);
                        }
                    },
                    [this](const AttackDodgedEvent& e) { handle_attack_dodged(e); },
                    [this](const ChatMsgEvent& e) { handle_chat_msg(e); },
                    [this](const EntityDiedEvent& e) { handle_entity_died(e); },
                    [this](const MeditationStartEvent&) {
                        audio_manager_.play_sfx("meditate");
                    },
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
                        if (!get_world_center(e.target_id, wx, wy))
                            return;
                        world_renderer_.sprites().trigger_spell_effect(e.effect_type, wx, wy);
                    },
                    [this](const GoldUpdateEvent& e) { player_stats_.gold = e.gold; },
                    [this](const NpcItemListEvent& e) {
                        merchant_controller_.on_item_list(e);
                        for (const auto& item : e.items)
                            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                                      item.item_name + ": " +
                                                              std::to_string(item.price) +
                                                              " de oro");
                    },
                    [this](const BankUpdateEvent& e) { handle_bank_update(e); },
                    [this](const ItemDroppedEvent& e) {
                        world_renderer_.ground_items().add_item(e.pos.x, e.pos.y, e.item_type,
                                                                e.item_name);
                    },
                    [this](const ItemPickedEvent& e) {
                        world_renderer_.ground_items().remove_item(e.pos.x, e.pos.y, e.item_name);
                    },
                    [](const auto&) {},
            },
            ev);
}

void ServerEventHandler::handle_entity_move(const EntityMoveEvent& e) {
    if (e.entity_id == player_stats_.player_id) {
        move_controller_.set_position(e.entity_pos.x, e.entity_pos.y);
        world_renderer_.sprites().move_movable(e.entity_pos.x, e.entity_pos.y);
        return;
    }
    world_renderer_.sprites().move_entity(e.entity_id, e.entity_pos.x, e.entity_pos.y);
    world_renderer_.sprites().mark_entity_moved(e.entity_id);
    world_renderer_.sprites().set_entity_src_y(e.entity_id,
                                               move_config_.body_src_y_for(e.entity_dir),
                                               move_config_.head_src_y_for(e.entity_dir));
    world_renderer_.sprites().advance_entity_src_x(e.entity_id, move_config_.walk_src_step,
                                                move_config_.walk_src_frames_for(e.entity_dir));
}

void ServerEventHandler::apply_equipment_overlays(uint16_t entity_id, bool is_local,
                                                   const ItemType equipped_types[EQUIP_SLOT_COUNT]) {
    static constexpr uint8_t overlay_slots[] = {
            static_cast<uint8_t>(EquipSlot::WEAPON),
            static_cast<uint8_t>(EquipSlot::HELMET),
            static_cast<uint8_t>(EquipSlot::SHIELD),
    };

    for (uint8_t slot : overlay_slots) {
        ItemType type = equipped_types[slot];
        auto it = config_.equip_overlays.find(static_cast<uint8_t>(type));
        if (type == ItemType::NONE || it == config_.equip_overlays.end()) {
            if (is_local)
                world_renderer_.sprites().clear_equipment_overlay(slot);
            else
                world_renderer_.sprites().clear_entity_equipment_overlay(entity_id, slot);
        } else {
            if (is_local)
                world_renderer_.sprites().update_equipment_overlay(
                        slot, it->second.path, it->second.offset_y, it->second.static_frame);
            else
                world_renderer_.sprites().update_entity_equipment_overlay(
                        entity_id, slot, it->second.path, it->second.offset_y,
                        it->second.static_frame);
        }
    }

    ItemType armor_type = equipped_types[static_cast<uint8_t>(EquipSlot::ARMOR)];
    auto armor_it = config_.equip_overlays.find(static_cast<uint8_t>(armor_type));
    if (armor_type == ItemType::NONE || armor_it == config_.equip_overlays.end()) {
        if (is_local)
            world_renderer_.sprites().reset_body_sprite();
        else
            world_renderer_.sprites().reset_entity_body_sprite(entity_id);
    } else {
        if (is_local)
            world_renderer_.sprites().set_body_sprite(armor_it->second.path);
        else
            world_renderer_.sprites().set_entity_body_sprite(entity_id, armor_it->second.path);
    }
}

void ServerEventHandler::handle_entity_spawn(const EntitySpawnEvent& e) {
    if (e.entity_id == player_stats_.player_id) {
        move_controller_.set_position(e.entity_pos.x, e.entity_pos.y);
        world_renderer_.sprites().move_movable(e.entity_pos.x, e.entity_pos.y);
        return;
    }
    world_renderer_.sprites().add_entity(e.entity_id, e.entity_pos.x, e.entity_pos.y,
                                          e.entity_name, e.entity_race, e.entity_class,
                                          e.sprite_id);
    if (e.entity_type != EntityType::NPC && !e.clan_name.empty())
        world_renderer_.sprites().set_entity_clan_name(e.entity_id, e.clan_name);
    world_renderer_.sprites().set_entity_src_y(e.entity_id,
                                               move_config_.body_src_y_for(e.entity_dir),
                                               move_config_.head_src_y_for(e.entity_dir));

    if (e.entity_type != EntityType::NPC) {
        const ItemType equipped_types[EQUIP_SLOT_COUNT] = {e.weapon_type, e.armor_type,
                                                           e.helmet_type, e.shield_type};
        apply_equipment_overlays(e.entity_id, false, equipped_types);
    }
}

void ServerEventHandler::handle_login_ok(const LoginOkEvent& e) {
    player_stats_.player_id = e.player_id;
    player_stats_.username = e.username;
    player_stats_.race = e.race;
    player_stats_.player_class = e.player_class;
    player_stats_.level = e.level;
    player_stats_.experience = e.experience;
    player_stats_.exp_to_next = e.exp_to_next;
    player_stats_.hp_current = e.hp_current;
    player_stats_.hp_max = e.hp_max;
    player_stats_.mana_current = e.mana_current;
    player_stats_.mana_max = e.mana_max;
    player_stats_.gold = e.gold;
    player_stats_.pos = e.pos;
    player_is_ghost_ = (e.hp_current == 0);
    move_controller_.set_position(e.pos.x, e.pos.y);
    world_renderer_.sprites().move_movable(e.pos.x, e.pos.y);
    world_renderer_.sprites().rebuild_local_player_sprites(e.race, e.player_class);
    if (player_is_ghost_)
        world_renderer_.sprites().set_movable_alpha(128);
}

void ServerEventHandler::handle_entity_despawn(const EntityDespawnEvent& e) {
    world_renderer_.sprites().remove_entity(e.entity_id);
}

void ServerEventHandler::handle_damage_received(const DamageReceivedEvent& e) {
    if (e.target_id == player_stats_.player_id) {
        player_stats_.hp_current = e.hp_current;
        player_stats_.hp_max = e.hp_max;
    }
    int wx, wy;
    if (!get_world_center(e.target_id, wx, wy))
        return;
    world_renderer_.sprites().trigger_damage_effect(wx, wy);
}

void ServerEventHandler::handle_attack_dodged(const AttackDodgedEvent& e) {
    if (e.player_id == player_stats_.player_id)
        chat_history_.add_message(ChatMsgType::SYSTEM, "", "Esquivaste el ataque");
    else
        chat_history_.add_message(ChatMsgType::SYSTEM, "", "El ataque fue esquivado");
}

void ServerEventHandler::handle_chat_msg(const ChatMsgEvent& e) {
    chat_history_.add_message(e.type, e.sender_name, e.message);
}

void ServerEventHandler::handle_clan_notification(const ClanNotificationEvent& e) {
    switch (e.type) {
        case ClanNotifType::MEMBER_ONLINE:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] " + e.username + " esta en linea");
            break;
        case ClanNotifType::MEMBER_OFFLINE:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] " + e.username + " se desconecto");
            break;
        case ClanNotifType::MEMBER_ATTACKED:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] " + e.username + " esta siendo atacado!");
            break;
        case ClanNotifType::JOIN_REQUEST:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] " + e.username + " quiere unirse al clan");
            break;
        case ClanNotifType::JOIN_ACCEPTED:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] Has sido aceptado en " + e.clan_name);
            break;
        case ClanNotifType::JOIN_REJECTED:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] Has sido rechazado de " + e.clan_name);
            break;
        case ClanNotifType::KICKED:
            chat_history_.add_message(ChatMsgType::SYSTEM, "",
                                      "[Clan] Has sido expulsado de " + e.clan_name);
            break;
    }
}

void ServerEventHandler::handle_clan_update(const ClanUpdateEvent& e) {
    world_renderer_.sprites().set_local_clan_name(e.clan_name);
    for (const auto& m : e.members)
        world_renderer_.sprites().set_entity_clan_by_username(m.username, e.clan_name);
    std::string msg = "--- Clan: " + e.clan_name + " ---";
    for (const auto& m : e.members) {
        msg += "\n  " + m.username;
        if (m.is_founder)
            msg += " (fundador)";
        msg += m.is_online ? " [En linea]" : " [Desconectado]";
    }
    chat_history_.add_message(ChatMsgType::SYSTEM, "", msg);
}

void ServerEventHandler::handle_inventory_update(const InventoryUpdateEvent& e) {
    player_stats_.inventory = e.slots;
}

void ServerEventHandler::handle_equip_update(const EquipUpdateEvent& e) {
    const bool is_local = (e.entity_id == player_stats_.player_id);

    if (is_local) {
        player_stats_.equipped[0] = e.weapon;
        player_stats_.equipped[1] = e.armor;
        player_stats_.equipped[2] = e.helmet;
        player_stats_.equipped[3] = e.shield;
    }

    const ItemType equipped_types[EQUIP_SLOT_COUNT] = {e.weapon.item_type, e.armor.item_type,
                                                       e.helmet.item_type, e.shield.item_type};
    apply_equipment_overlays(e.entity_id, is_local, equipped_types);
}

void ServerEventHandler::handle_entity_died(const EntityDiedEvent& e) {
    if (e.entity_id != player_stats_.player_id) {
        if (world_renderer_.sprites().is_npc(e.entity_id)) {
            world_renderer_.sprites().remove_entity(e.entity_id);
        } else {
            world_renderer_.sprites().set_entity_alpha(e.entity_id, 128);
        }
        return;
    }
    audio_manager_.play_sfx("death");
    player_is_ghost_ = true;
    world_renderer_.sprites().set_entity_alpha(e.entity_id, 128);
    world_renderer_.sprites().set_movable_alpha(128);
}

void ServerEventHandler::handle_player_respawned(const PlayerRespawnedEvent& e) {
    world_renderer_.sprites().set_entity_alpha(e.entity_id, 255);
    if (e.entity_id != player_stats_.player_id) {
        return;
    }
    player_is_ghost_ = false;
    player_stats_.hp_current = e.hp_current;
    player_stats_.hp_max = e.hp_max;
    world_renderer_.sprites().set_movable_alpha(255);
}

void ServerEventHandler::handle_heal_received(const HealReceivedEvent& e) {
    if (e.player_id != player_stats_.player_id)
        return;
    player_stats_.hp_current = e.hp_current;
    player_stats_.mana_current = e.mana_current;
}

void ServerEventHandler::handle_map_transition(const MapTransitionEvent& e) {
    auto it = config_.tilemap_configs.find(e.map_name);
    if (it == config_.tilemap_configs.end())
        return;

    current_map_name_ = e.map_name;
    world_renderer_.sprites().clear_entities();
    world_renderer_.ground_items().clear();
    world_renderer_.load_map(it->second);
    world_renderer_.sprites().move_movable(e.pos_x, e.pos_y);
    move_controller_.set_position(e.pos_x, e.pos_y);
    player_stats_.pos = {e.pos_x, e.pos_y};
}

void ServerEventHandler::handle_bank_update(const BankUpdateEvent& e) {
    chat_history_.add_message(ChatMsgType::SYSTEM, "", "Banco - Oro: " + std::to_string(e.gold));
    for (const auto& slot : e.slots) {
        if (slot.item_type != ItemType::NONE)
            chat_history_.add_message(ChatMsgType::SYSTEM, "", "Banco - " + slot.item_name);
    }
}

void ServerEventHandler::handle_player_stats(const PlayerStatsEvent& e) {
    if (e.level > player_stats_.level) {
        audio_manager_.play_sfx("level_up");
    }
    player_stats_.level = e.level;
    player_stats_.experience = e.experience;
    player_stats_.exp_to_next = e.exp_to_next;
    player_stats_.hp_current = e.hp_current;
    player_stats_.hp_max = e.hp_max;
    player_stats_.mana_current = e.mana_current;
    player_stats_.mana_max = e.mana_max;
    player_stats_.crit_chance = e.crit_chance;
    player_stats_.damage_min = e.damage_min;
    player_stats_.damage_max = e.damage_max;
    player_stats_.defense_min = e.defense_min;
    player_stats_.defense_max = e.defense_max;
    player_stats_.dodge_chance = e.dodge_chance;
    player_stats_.strength = e.strength;
    player_stats_.agility = e.agility;
}

bool ServerEventHandler::get_world_center(uint16_t entity_id, int& wx, int& wy) const {
    if (entity_id == player_stats_.player_id) {
        world_renderer_.sprites().get_movable_position(wx, wy);
        wx += world_renderer_.sprites().movable_w() / 2;
        wy += world_renderer_.sprites().movable_h() / 2;
        return true;
    }
    return world_renderer_.sprites().get_entity_world_position(entity_id, wx, wy);
}

void ServerEventHandler::play_spatial_sfx(const std::string& name, uint16_t entity_id) {
    int sx, sy;
    if (world_renderer_.sprites().get_entity_world_position(entity_id, sx, sy)) {
        audio_manager_.play_sfx_at(name, sx, sy);
    } else {
        audio_manager_.play_sfx(name);
    }
}
