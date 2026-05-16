#include "server_protocol.h"

#include <stdexcept>
#include <utility>

#include "../common/socket.h"

ServerProtocol::ServerProtocol(Socket&& skt): skt(std::move(skt)), protocol(this->skt) {}

ClientCommand ServerProtocol::recv_command() {
    OpCode opcode = protocol.recv_opcode();

    switch (opcode) {
        case OpCode::MOVE:
            return recv_move();
        default:
            throw std::runtime_error("Unknown command opcode: " +
                                     std::to_string(static_cast<int>(opcode)));
    }
}

ClientCommand ServerProtocol::recv_move() {
    Direction dir = static_cast<Direction>(protocol.recv_uint8());
    return MoveCmd{dir};
}

/**
 * Creates a struct that inherits from all the lambdas passed to it.
 * Each lambda has its own operator(), and using Ts::operator()...;
 * brings all of them into scope. When std::visit calls operator()
 * on the overloaded object, overload resolution picks the lambda whose parameter type matches.
 */
template <class... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};
/**
 * Tells the compiler: "when you write overloaded{lambda1, lambda2, ...},
 * deduce the template arguments from the lambdas' types"
 */
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void ServerProtocol::send_event(const ServerEvent& ev) {
    std::visit(overloaded{
                       [this](const LoginOkEvent& msg) { send_login_ok(msg); },
                       [this](const LoginErrorEvent& msg) { send_login_error(msg); },
                       [this](const CharacterCreatedEvent& msg) { send_character_created(msg); },
                       [this](const CharacterErrorEvent& msg) { send_character_error(msg); },
                       [this](const MapInfoEvent& msg) { send_map_info(msg); },
                       [this](const PlayerStatsEvent& msg) { send_player_stats(msg); },
                       [this](const EntitySpawnEvent& msg) { send_entity_spawn(msg); },
                       [this](const EntityDespawnEvent& msg) { send_entity_despawn(msg); },
                       [this](const EntityMoveEvent& msg) { send_entity_move(msg); },
                       [this](const DamageDealtEvent& msg) { send_damage_dealt(msg); },
                       [this](const DamageReceivedEvent& msg) { send_damage_received(msg); },
                       [this](const AttackDodgedEvent& msg) { send_attack_dodged(msg); },
                       [this](const EntityDiedEvent& msg) { send_entity_died(msg); },
                       [this](const PlayerRespawnedEvent& msg) { send_player_respawned(msg); },
                       [this](const MeditationStartEvent&) { send_meditation_start(); },
                       [this](const MeditationStopEvent&) { send_meditation_stop(); },
                       [this](const InventoryUpdateEvent& msg) { send_inventory_update(msg); },
                       [this](const EquipUpdateEvent& msg) { send_equip_update(msg); },
                       [this](const GoldUpdateEvent& msg) { send_gold_update(msg); },
                       [this](const ItemDroppedEvent& msg) { send_item_dropped(msg); },
                       [this](const ItemPickedEvent& msg) { send_item_picked(msg); },
                       [this](const NpcItemListEvent& msg) { send_npc_item_list(msg); },
                       [this](const TransactionOkEvent& msg) { send_transaction_ok(msg); },
                       [this](const TransactionErrorEvent& msg) { send_transaction_error(msg); },
                       [this](const ChatMsgEvent& msg) { send_chat_msg(msg); },
                       [this](const ClanNotificationEvent& msg) { send_clan_notification(msg); },
                       [this](const ClanUpdateEvent& msg) { send_clan_update(msg); },
                       [this](const ServerMsgEvent& msg) { send_server_msg(msg); },
               },
               ev);
}
