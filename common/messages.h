#ifndef MESSAGES_H_
#define MESSAGES_H_

/*
 * messages.h
 *
 * Define todos los tipos de datos que viajan entre cliente y servidor.
 * Este archivo es compartido por client/, server/ y tests/.
 *
 * Convenciones:
 *  - Los structs reflejan exactamente los mensajes del protocolo (protocol.md).
 *  - Command  = cliente -> servidor.
 *  - Event    = servidor -> cliente.
 *  - ClientCommand y ServerEvent son std::variant de todos los tipos concretos.
 *    El receptor hace std::visit para despachar.
 */

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// ---------------------------------------------------------------------------
// Enums (sección 7 de protocol.md)
// ---------------------------------------------------------------------------

enum class Race : uint8_t { HUMAN = 0x01, ELF = 0x02, DWARF = 0x03, GNOME = 0x04 };

enum class PlayerClass : uint8_t { MAGE = 0x01, CLERIC = 0x02, PALADIN = 0x03, WARRIOR = 0x04 };

enum class Direction : uint8_t { NORTH = 0x00, SOUTH = 0x01, EAST = 0x02, WEST = 0x03 };

enum class ItemType : uint8_t {
    NONE = 0x00,
    SWORD = 0x01,
    AXE = 0x02,
    HAMMER = 0x03,
    ASH_STAFF = 0x04,
    ELVEN_FLUTE = 0x05,
    KNOTTED_STAFF = 0x06,
    STUDDED_STAFF = 0x07,
    SIMPLE_BOW = 0x08,
    COMPOSITE_BOW = 0x09,
    LEATHER_ARMOR = 0x0A,
    PLATE_ARMOR = 0x0B,
    BLUE_TUNIC = 0x0C,
    HOOD = 0x0D,
    IRON_HELMET = 0x0E,
    TURTLE_SHIELD = 0x0F,
    IRON_SHIELD = 0x10,
    MAGIC_HAT = 0x11,
    HEALTH_POTION = 0x12,
    MANA_POTION = 0x13,
    GOLD_DROP = 0x14
};

enum class EquipSlot : uint8_t {
    WEAPON = 0x00,
    ARMOR = 0x01,
    HELMET = 0x02,
    SHIELD = 0x03,
    CONSUMABLE = 0x04,
    NONE = 0xFF
};

constexpr uint8_t EQUIP_SLOT_COUNT = 4;

enum class EntityType : uint8_t { PLAYER = 0x00, NPC = 0x01 };

enum class LoginError : uint8_t {
    INVALID_CREDENTIALS = 0x01,
    ALREADY_LOGGED_IN = 0x02,
    SERVER_FULL = 0x03
};

enum class CharacterError : uint8_t { USERNAME_TAKEN = 0x01, INVALID_USERNAME = 0x02 };

enum class TransactionType : uint8_t {
    BUY = 0x01,
    SELL = 0x02,
    DEPOSIT = 0x03,
    WITHDRAW = 0x04,
    HEAL = 0x05
};

enum class ChatMsgType : uint8_t { SYSTEM = 0x00, PRIVATE = 0x01, CLAN = 0x02, SAY = 0x03 };

enum class ClanNotifType : uint8_t {
    MEMBER_ONLINE = 0x00,
    MEMBER_OFFLINE = 0x01,
    MEMBER_ATTACKED = 0x02,
    JOIN_REQUEST = 0x03,
    JOIN_ACCEPTED = 0x04,
    JOIN_REJECTED = 0x05,
    KICKED = 0x06
};

enum class ServerMsgSeverity : uint8_t { INFO = 0x00, WARNING = 0x01, ERROR = 0x02 };

enum class TileType : uint8_t {
    GRASS = 0x00,
    SAND = 0x01,
    FOREST = 0x02,
    WATER = 0x03,
    ROAD = 0x04,
    DUNGEON_FLOOR = 0x05,
    WALL = 0x06,
    BUILDING_WALL = 0x07
};

// ---------------------------------------------------------------------------
// Tipos de struct
// ---------------------------------------------------------------------------

struct Position {
    uint16_t x = 0;
    uint16_t y = 0;
};

struct InventorySlot {
    uint8_t slot_index = 0;
    ItemType item_type = ItemType::NONE;
    std::string item_name;
};

struct TileInfo {
    uint16_t x = 0;
    uint16_t y = 0;
    TileType type = TileType::GRASS;
    bool walkable = true;
};

struct NpcItemEntry {
    std::string item_name;
    ItemType item_type = ItemType::NONE;
    uint8_t sprite_id = 0;
    uint32_t price = 0;
};

struct ClanMember {
    std::string username;
    bool is_founder = false;
    bool is_online = false;
};

// ---------------------------------------------------------------------------
// Comandos: Cliente -> Servidor (sección 3.1 y 4 de protocol.md)
// ---------------------------------------------------------------------------

// 0x01
struct LoginCmd {
    std::string username;
    std::string password;
};

// 0x02
struct CreateCharacterCmd {
    std::string username;
    std::string password;
    Race race;
    PlayerClass player_class;
};

// 0x03
struct MoveCmd {
    Direction direction;
};

// 0x04
struct AttackCmd {
    uint16_t target_id;
};

// 0x05
struct CastSpellCmd {
    uint16_t target_id = 0;
};

// 0x06
struct PickupItemCmd {};

// 0x07
struct DropItemCmd {};

// 0x08
struct EquipItemCmd {
    uint8_t slot_index = 0;
};

// 0x09
struct UnequipItemCmd {
    EquipSlot slot = EquipSlot::WEAPON;
};

// 0x0A
struct MeditateCmd {};

// 0x0B
struct ResurrectCmd {};

// 0x0C
struct NpcBuyCmd {
    std::string item_name;
};

// 0x0D
struct NpcSellCmd {
    std::string item_name;
};

// 0x0E
struct NpcHealCmd {};

// 0x0F
struct BankDepositCmd {};

// 0x10
struct BankWithdrawCmd {};

// 0x11
struct NpcListCmd {};

// 0x12
struct PrivateMsgCmd {
    std::string target_nick;
    std::string message;
};

// 0x13 – 0x1A  Clanes
struct ClanFoundCmd { std::string clan_name; };
struct ClanJoinRequestCmd { std::string clan_name; };
struct ClanReviewCmd {};
struct ClanAcceptCmd { std::string target_nick; };
struct ClanRejectCmd { std::string target_nick; };
struct ClanBanCmd { std::string target_nick; };
struct ClanKickCmd { std::string target_nick; };
struct ClanLeaveCmd {};

// 0x1E
struct SendChatMsgCmd {
    std::string text;
};

// 0x1B – 0x1D, 0x1F-0x20  Cheats
struct CheatInfiniteHpCmd {};
struct CheatInfiniteManaCmd {};
struct CheatDieCmd {};
struct CheatLevelUpCmd {};
struct CheatLevelDownCmd {};
struct CheatAddGoldCmd {};
struct CheatResetGoldCmd {};
struct CheatVelocityCmd {};
struct CheatReviveCmd {};
struct CheatFillInventoryCmd {};
struct CheatClearInventoryCmd {};
struct CheatResetManaCmd {};

struct ChangeMapCmd {
    std::string prop_name;
};

/*
 * ClientCommand es la variante que engloba todos los comandos.
 * El GameLoop hace std::visit sobre esta variante para despacharlos.
 */
using ClientCommand = std::variant<
        LoginCmd, CreateCharacterCmd, MoveCmd, AttackCmd, CastSpellCmd, PickupItemCmd, DropItemCmd,
        EquipItemCmd, UnequipItemCmd, MeditateCmd, ResurrectCmd, NpcBuyCmd, NpcSellCmd, NpcHealCmd,
        BankDepositCmd, BankWithdrawCmd, NpcListCmd, PrivateMsgCmd, SendChatMsgCmd, ClanFoundCmd,
        ClanJoinRequestCmd, ClanReviewCmd, ClanAcceptCmd, ClanRejectCmd, ClanBanCmd, ClanKickCmd,
        ClanLeaveCmd, CheatInfiniteHpCmd, CheatInfiniteManaCmd, CheatDieCmd, CheatLevelUpCmd,
        CheatLevelDownCmd, CheatAddGoldCmd, CheatResetGoldCmd, CheatVelocityCmd, CheatReviveCmd,
        CheatFillInventoryCmd, CheatClearInventoryCmd, CheatResetManaCmd, ChangeMapCmd>;

// ---------------------------------------------------------------------------
// Eventos: Servidor -> Cliente (sección 3.2 y 5 de protocol.md)
// ---------------------------------------------------------------------------

// 0x80
struct LoginOkEvent {
    uint16_t player_id;
    std::string username;
    Race race;
    PlayerClass player_class;
    uint8_t level;
    uint32_t experience;
    uint32_t exp_to_next = 0;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t mana_current;
    uint32_t mana_max;
    uint32_t gold;
    Position pos;
};

// 0x81
struct LoginErrorEvent {
    LoginError error_code;
    std::string message;
};

// 0x82
struct CharacterCreatedEvent {
    LoginOkEvent data;
};

// 0x83
struct CharacterErrorEvent {
    CharacterError error_code;
    std::string message;
};

// 0x84
struct MapInfoEvent {};

// 0x85
struct PlayerStatsEvent {
    uint8_t level;
    uint32_t experience;
    uint32_t exp_to_next;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t mana_current;
    uint32_t mana_max;
    uint8_t crit_chance;
    uint16_t damage_min;
    uint16_t damage_max;
    uint16_t defense_min;
    uint16_t defense_max;
    uint8_t dodge_chance;
    uint16_t strength;
    uint16_t agility;
};

// 0x86
struct EntitySpawnEvent {
    uint16_t entity_id;
    EntityType entity_type;
    Position entity_pos;
    Direction entity_dir;
    std::string entity_name;
    Race entity_race;
    PlayerClass entity_class;
    ItemType weapon_type = ItemType::NONE;
    ItemType armor_type = ItemType::NONE;
    ItemType helmet_type = ItemType::NONE;
    ItemType shield_type = ItemType::NONE;
};

// 0x87
struct EntityDespawnEvent {
    uint16_t entity_id;
};

// 0x88
struct EntityMoveEvent {
    uint16_t entity_id;
    Position entity_pos;
    Direction entity_dir;
};

// 0x89
struct DamageDealtEvent {
    uint16_t target_id;
    uint32_t damage;
};

// 0x8A
struct DamageReceivedEvent {
    uint16_t target_id;
    uint16_t attacker_id;
    uint32_t damage;
    uint32_t hp_current;
    uint32_t hp_max;
};

// 0x8B
struct AttackDodgedEvent {
    uint16_t player_id;
};

// 0x8C
struct EntityDiedEvent {
    uint16_t entity_id;
};

// 0x8D
struct PlayerRespawnedEvent {
    uint16_t entity_id;
    uint32_t hp_current;
    uint32_t hp_max;
};

// 0x8E / 0x8F
struct MeditationStartEvent {};
struct MeditationStopEvent {};

// 0x90
struct InventoryUpdateEvent {
    std::vector<InventorySlot> slots;
};

// 0x91
struct EquipUpdateEvent {
    uint16_t entity_id = 0;
    InventorySlot weapon;
    InventorySlot armor;
    InventorySlot helmet;
    InventorySlot shield;
};

// 0x92
struct GoldUpdateEvent {
    uint32_t gold;
};

// 0x93
struct ItemDroppedEvent {};

// 0x94
struct ItemPickedEvent {};

// 0x95
struct NpcItemListEvent {
    std::vector<NpcItemEntry> items;
};

// 0x96
struct TransactionOkEvent {};

// 0x97
struct TransactionErrorEvent {};

// 0x98
struct ChatMsgEvent {
    ChatMsgType type;
    std::string sender_name;
    std::string message;
    uint16_t recipient_id = 0;
    uint16_t sender_id = 0;
};

// 0x99
struct ClanNotificationEvent {
    ClanNotifType type;
    std::string username;
    std::string clan_name;
};

// 0x9A
struct ClanUpdateEvent {
    std::string clan_name;
    std::vector<ClanMember> members;
};

// 0x9B
struct ServerMsgEvent {};

// 0x9C
struct MapTransitionEvent {
    std::string map_name;
    uint16_t pos_x;
    uint16_t pos_y;
};

struct SpellEffectEvent {
    uint16_t target_id;
    uint8_t effect_type;  // 0 = HEAL
};

struct HealReceivedEvent {
    uint16_t player_id;
    uint32_t hp_current;
    uint32_t mana_current;
};

/*
 * ServerEvent es la variante que engloba todos los eventos.
 * El cliente hace std::visit sobre esta variante para renderizar/actualizar estado.
 */
using ServerEvent =
        std::variant<LoginOkEvent, LoginErrorEvent, CharacterCreatedEvent, CharacterErrorEvent,
                     MapInfoEvent, PlayerStatsEvent, EntitySpawnEvent, EntityDespawnEvent,
                     EntityMoveEvent, DamageDealtEvent, DamageReceivedEvent, AttackDodgedEvent,
                     EntityDiedEvent, PlayerRespawnedEvent, MeditationStartEvent,
                     MeditationStopEvent, InventoryUpdateEvent, EquipUpdateEvent, GoldUpdateEvent,
                     ItemDroppedEvent, ItemPickedEvent, NpcItemListEvent, TransactionOkEvent,
                     TransactionErrorEvent, ChatMsgEvent, ClanNotificationEvent, ClanUpdateEvent,
                     ServerMsgEvent, MapTransitionEvent, HealReceivedEvent, SpellEffectEvent>;

#endif  // MESSAGES_H_
