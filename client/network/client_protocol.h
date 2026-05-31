#ifndef CLIENT_PROTOCOL_H_
#define CLIENT_PROTOCOL_H_

#include "../../common/messages.h"
#include "../../common/protocol.h"

class ClientProtocol {
public:
    explicit ClientProtocol(Socket&& skt);

    // Envío de comandos (Cliente -> Servidor)
    void send_login(const LoginCmd& cmd);
    void send_create_character(const CreateCharacterCmd& cmd);
    void send_move(const MoveCmd& cmd);

    // TODO ...
    void send_attack(const AttackCmd& cmd);
    void send_cast_spell(const CastSpellCmd& cmd);
    void send_pickup_item();
    void send_drop_item(const DropItemCmd& cmd);
    void send_equip_item(const EquipItemCmd& cmd);
    void send_unequip_item(const UnequipItemCmd& cmd);
    void send_meditate();
    void send_resurrect();
    void send_npc_buy(const NpcBuyCmd& cmd);
    void send_npc_sell(const NpcSellCmd& cmd);
    void send_npc_heal(const NpcHealCmd& cmd);
    void send_bank_deposit(const BankDepositCmd& cmd);
    void send_bank_withdraw(const BankWithdrawCmd& cmd);
    void send_npc_list(const NpcListCmd& cmd);
    void send_private_msg(const PrivateMsgCmd& cmd);
    void send_chat_msg(const SendChatMsgCmd& cmd);
    void send_clan_found(const ClanFoundCmd& cmd);
    void send_clan_join_request(const ClanJoinRequestCmd& cmd);
    void send_clan_review();
    void send_clan_accept(const ClanAcceptCmd& cmd);
    void send_clan_reject(const ClanRejectCmd& cmd);
    void send_clan_ban(const ClanBanCmd& cmd);
    void send_clan_kick(const ClanKickCmd& cmd);
    void send_clan_leave();

    // Cheats
    void send_cheat_infinite_hp();
    void send_cheat_infinite_mana();
    void send_cheat_die();
    void send_cheat_level_up();
    void send_cheat_level_down();
    void send_change_map(const ChangeMapCmd& cmd);

    // Envía cualquier comando usando std::visit.
    void send_command(const ClientCommand& cmd);

    // Recibe y deserializa el próximo evento del servidor.
    ServerEvent recv_event();

    void shutdown();

    ClientProtocol(const ClientProtocol&) = delete;
    ClientProtocol& operator=(const ClientProtocol&) = delete;

private:
    Socket skt;
    Protocol protocol;

    // Deserializadores privados por tipo de evento
    LoginOkEvent recv_login_payload();
    ServerEvent recv_login_ok();
    ServerEvent recv_login_error();
    ServerEvent recv_character_created();
    ServerEvent recv_character_error();
    ServerEvent recv_entity_spawn();
    ServerEvent recv_entity_despawn();
    ServerEvent recv_entity_move();
    ServerEvent recv_damage_dealt();
    ServerEvent recv_damage_received();
    ServerEvent recv_entity_died();
    ServerEvent recv_player_respawned();
    ServerEvent recv_chat_msg();
    ServerEvent recv_clan_notification();
    ServerEvent recv_clan_update();
    ServerEvent recv_heal_received();
};

#endif  // CLIENT_PROTOCOL_H_
