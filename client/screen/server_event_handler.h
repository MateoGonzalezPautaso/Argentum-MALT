#ifndef CLIENT_SERVER_EVENT_HANDLER_H
#define CLIENT_SERVER_EVENT_HANDLER_H

#include <string>

#include "../../common/messages.h"
#include "../audio/audio_manager.h"
#include "../chat/chat_history.h"
#include "../config/config.h"
#include "../config/player_stats.h"
#include "../input/move_controller.h"
#include "../render/world_renderer.h"

class MerchantController;

class ServerEventHandler {
public:
    ServerEventHandler(PlayerStats& player_stats, WorldRenderer& world_renderer,
                       AudioManager& audio_manager, ChatHistory& chat_history,
                       MoveController& move_controller, const ClientConfig& config,
                       const MoveConfig& move_config, MerchantController& merchant_controller,
                       bool& player_is_ghost, std::string& current_map_name);

    void apply(const ServerEvent& ev);

private:
    PlayerStats& player_stats_;
    WorldRenderer& world_renderer_;
    AudioManager& audio_manager_;
    ChatHistory& chat_history_;
    MoveController& move_controller_;
    const ClientConfig& config_;
    const MoveConfig& move_config_;
    MerchantController& merchant_controller_;
    bool& player_is_ghost_;
    std::string& current_map_name_;

    void handle_entity_move(const EntityMoveEvent& e);
    void handle_entity_spawn(const EntitySpawnEvent& e);
    void handle_entity_despawn(const EntityDespawnEvent& e);
    void handle_login_ok(const LoginOkEvent& e);
    void handle_damage_received(const DamageReceivedEvent& e);
    void handle_attack_dodged(const AttackDodgedEvent& e);
    void handle_chat_msg(const ChatMsgEvent& e);
    void handle_entity_died(const EntityDiedEvent& e);
    void handle_player_respawned(const PlayerRespawnedEvent& e);
    void handle_clan_notification(const ClanNotificationEvent& e);
    void handle_clan_update(const ClanUpdateEvent& e);
    void handle_map_transition(const MapTransitionEvent& e);
    void handle_heal_received(const HealReceivedEvent& e);
    void handle_inventory_update(const InventoryUpdateEvent& e);
    void handle_equip_update(const EquipUpdateEvent& e);
    void handle_player_stats(const PlayerStatsEvent& e);
    void handle_bank_update(const BankUpdateEvent& e);

    void play_spatial_sfx(const std::string& name, uint16_t entity_id);
    void apply_equipment_overlays(uint16_t entity_id, bool is_local,
                                  const ItemType equipped_types[EQUIP_SLOT_COUNT]);
};

#endif  // CLIENT_SERVER_EVENT_HANDLER_H
