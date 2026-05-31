#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <string>

#include "socket.h"

enum class OpCode : uint8_t {
    // Commands (client -> server) 0x01 - 0x1D
    LOGIN = 0x01,
    CREATE_CHARACTER = 0x02,
    MOVE = 0x03,
    ATTACK = 0x04,
    CAST_SPELL = 0x05,
    PICKUP_ITEM = 0x06,
    DROP_ITEM = 0x07,
    EQUIP_ITEM = 0x08,
    UNEQUIP_ITEM = 0x09,
    MEDITATE = 0x0A,
    RESURRECT = 0x0B,
    NPC_BUY = 0x0C,
    NPC_SELL = 0x0D,
    NPC_HEAL = 0x0E,
    BANK_DEPOSIT = 0x0F,
    BANK_WITHDRAW = 0x10,
    NPC_LIST = 0x11,
    PRIVATE_MSG = 0x12,
    CLAN_FOUND = 0x13,
    CLAN_JOIN_REQUEST = 0x14,
    CLAN_REVIEW = 0x15,
    CLAN_ACCEPT = 0x16,
    CLAN_REJECT = 0x17,
    CLAN_BAN = 0x18,
    CLAN_KICK = 0x19,
    CLAN_LEAVE = 0x1A,
    CHEAT_INFINITE_HP = 0x1B,
    CHEAT_INFINITE_MANA = 0x1C,
    CHEAT_DIE = 0x1D,
    SEND_CHAT = 0x1E,
    CHEAT_LEVEL_UP = 0x1F,
    CHEAT_LEVEL_DOWN = 0x20,
    // Events (server -> client) 0x80 - 0x9B
    LOGIN_OK = 0x80,
    LOGIN_ERROR = 0x81,
    CHARACTER_CREATED = 0x82,
    CHARACTER_ERROR = 0x83,
    MAP_INFO = 0x84,
    PLAYER_STATS = 0x85,
    ENTITY_SPAWN = 0x86,
    ENTITY_DESPAWN = 0x87,
    ENTITY_MOVE = 0x88,
    DAMAGE_DEALT = 0x89,
    DAMAGE_RECEIVED = 0x8A,
    ATTACK_DODGED = 0x8B,
    ENTITY_DIED = 0x8C,
    PLAYER_RESPAWNED = 0x8D,
    MEDITATION_START = 0x8E,
    MEDITATION_STOP = 0x8F,
    INVENTORY_UPDATE = 0x90,
    EQUIP_UPDATE = 0x91,
    GOLD_UPDATE = 0x92,
    ITEM_DROPPED = 0x93,
    ITEM_PICKED = 0x94,
    NPC_ITEM_LIST = 0x95,
    TRANSACTION_OK = 0x96,
    TRANSACTION_ERROR = 0x97,
    CHAT_MSG = 0x98,
    CLAN_NOTIFICATION = 0x99,
    CLAN_UPDATE = 0x9A,
    SERVER_MSG = 0x9B,
    HEAL_RECEIVED = 0x9C,
};

class Protocol {
public:
    explicit Protocol(Socket& skt);

    /* TODO
    uint32_t    recv_uint32();
    std::string recv_string();
    bool        recv_bool();

    hacer las versiones send tambien
    */

    void send_uint8(uint8_t value);
    uint8_t recv_uint8();

    void send_uint16(uint16_t value);
    uint16_t recv_uint16();

    void send_str(const std::string& message);
    std::string recv_str();

    void send_opcode(OpCode opcode);
    OpCode recv_opcode();

    Protocol(const Protocol&) = delete;
    Protocol& operator=(const Protocol&) = delete;

    void send_uint32(uint32_t value);
    uint32_t recv_uint32();

    void send_bool(bool value);
    bool recv_bool();

private:
    Socket& skt;
};

#endif
