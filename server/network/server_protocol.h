#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_

#include "../../common/messages.h"
#include "../../common/protocol.h"

class ServerProtocol {
public:
    explicit ServerProtocol(Socket&& skt);

    // Envío de eventos (Servidor -> Cliente)
    void send_login_ok(const LoginOkEvent& ev);
    void send_login_error(const LoginErrorEvent& ev);
    void send_character_created(const CharacterCreatedEvent& ev);
    void send_character_error(const CharacterErrorEvent& ev);
    void send_map_info(const MapInfoEvent& ev);
    void send_player_stats(const PlayerStatsEvent& ev);
    void send_entity_spawn(const EntitySpawnEvent& ev);
    void send_entity_despawn(const EntityDespawnEvent& ev);
    void send_entity_move(const EntityMoveEvent& ev);
    void send_damage_dealt(const DamageDealtEvent& ev);
    void send_damage_received(const DamageReceivedEvent& ev);
    void send_attack_dodged(const AttackDodgedEvent& ev);
    void send_entity_died(const EntityDiedEvent& ev);
    void send_player_respawned(const PlayerRespawnedEvent& ev);
    void send_meditation_start();
    void send_meditation_stop();
    void send_inventory_update(const InventoryUpdateEvent& ev);
    void send_equip_update(const EquipUpdateEvent& ev);
    void send_gold_update(const GoldUpdateEvent& ev);
    void send_item_dropped(const ItemDroppedEvent& ev);
    void send_item_picked(const ItemPickedEvent& ev);
    void send_npc_item_list(const NpcItemListEvent& ev);
    void send_transaction_ok(const TransactionOkEvent& ev);
    void send_transaction_error(const TransactionErrorEvent& ev);
    void send_chat_msg(const ChatMsgEvent& ev);
    void send_clan_notification(const ClanNotificationEvent& ev);
    void send_clan_update(const ClanUpdateEvent& ev);
    void send_server_msg(const ServerMsgEvent& ev);
    void send_map_transition(const MapTransitionEvent& ev);
    void send_heal_received(const HealReceivedEvent& ev);
    void send_spell_effect(const SpellEffectEvent& ev);

    // Envía cualquier evento usando std::visit.
    void send_event(const ServerEvent& ev);

    // Recibe y deserializa el próximo comando del cliente.
    ClientCommand recv_command();

    void shutdown();

    ServerProtocol(const ServerProtocol&) = delete;
    ServerProtocol& operator=(const ServerProtocol&) = delete;

private:
    Socket skt;
    Protocol protocol;

    // Envío de eventos (Servidor -> Cliente)
    void send_login_payload(const LoginOkEvent& ev);

    // Deserializadores privados por tipo de comando
    ClientCommand recv_login();
    ClientCommand recv_create_character();
    ClientCommand recv_move();
    ClientCommand recv_attack();
    ClientCommand recv_send_chat_msg();
    ClientCommand recv_change_map();
    ClientCommand recv_cast_spell();
    ClientCommand recv_equip_item();
    ClientCommand recv_unequip_item();
    ClientCommand recv_npc_buy();
    ClientCommand recv_npc_sell();
};

#endif  // SERVER_PROTOCOL_H_
