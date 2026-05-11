#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_

#include "../common/protocol.h"
#include "../common/messages.h"

/*
 * Protocolo desde la perspectiva del servidor:
 *   - recv_command  deserializa el próximo comando del cliente.
 *   - send_*        serializa y envía un evento al cliente.
 */
class ServerProtocol : public Protocol {
private:
    // Deserializadores privados por tipo de comando
    ClientCommand recv_login();
    ClientCommand recv_create_character();
    ClientCommand recv_move();
    ClientCommand recv_attack();
    ClientCommand recv_cast_spell();
    ClientCommand recv_drop_item();
    ClientCommand recv_equip_item();
    ClientCommand recv_unequip_item();
    ClientCommand recv_npc_buy();
    ClientCommand recv_npc_sell();
    ClientCommand recv_npc_heal();
    ClientCommand recv_bank_deposit();
    ClientCommand recv_bank_withdraw();
    ClientCommand recv_npc_list();
    ClientCommand recv_private_msg();
    ClientCommand recv_clan_found();
    ClientCommand recv_clan_join_request();
    ClientCommand recv_clan_accept();
    ClientCommand recv_clan_reject();
    ClientCommand recv_clan_ban();
    ClientCommand recv_clan_kick();

public:
    ServerProtocol();
    ~ServerProtocol() = default;

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

    // Envía cualquier evento usando std::visit.
    void send_event(const ServerEvent& ev);

    // Recibe y deserializa el próximo comando del cliente, retornándolo como ClientCommand.
    ClientCommand recv_command();

    ServerProtocol(const ServerProtocol&) = delete;
    ServerProtocol& operator=(const ServerProtocol&) = delete;
};

#endif  // SERVER_PROTOCOL_H_
