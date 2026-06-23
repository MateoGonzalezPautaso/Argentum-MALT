#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client/network/client_protocol.h"
#include "common/messages.h"
#include "common/socket.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "server/network/server_protocol.h"

/*
 * ProtocolTest is the base class for all protocol tests.
 *
 * socketpair(AF_UNIX, SOCK_STREAM, ...) creates a pair of connected file
 * descriptors: bytes written to fds[0] can be read from fds[1] and vice versa.
 * It is the local equivalent of a TCP connection: same byte-stream semantics,
 * no network involved.
 *
 * Each TEST_F gets fresh file descriptors because SetUp() runs before every
 * test and TearDown() runs after.
 */
class ProtocolTest: public ::testing::Test {
protected:
    int fds[2];

    void SetUp() override { ASSERT_NE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), -1); }

    void TearDown() override {
        // If the Socket objects already took ownership and closed the fds,
        // these calls return -1 silently. That is not an error.
        ::close(fds[0]);
        ::close(fds[1]);
    }
};

// ─────────────────────────────────────────────────────────────
// MOVE command
// ─────────────────────────────────────────────────────────────

/*
 * WithParamInterface<Direction> adds GetParam() to the class, which returns
 * the current parameter value for this test run.
 * TEST_P defines the test body; INSTANTIATE_TEST_SUITE_P runs it once per
 * value in Values(...).
 */
class ProtocolMoveTest: public ProtocolTest, public ::testing::WithParamInterface<Direction> {};

TEST_P(ProtocolMoveTest, RoundtripAllDirections) {
    Socket client_skt = Socket::from_fd(fds[0]);
    Socket server_skt = Socket::from_fd(fds[1]);

    ClientProtocol client(std::move(client_skt));
    ServerProtocol server(std::move(server_skt));

    Direction dir = GetParam();
    client.send_command(MoveCmd{dir});

    ClientCommand cmd = server.recv_command();
    // std::holds_alternative<T>(v) asks: "is the active type in this variant T?"
    // ASSERT stops the test on failure, preventing std::bad_variant_access in the get below.
    ASSERT_TRUE(std::holds_alternative<MoveCmd>(cmd));
    EXPECT_EQ(std::get<MoveCmd>(cmd).direction, dir);
}

// Generates one independent test per direction: 0=NORTH, 1=SOUTH, 2=EAST, 3=WEST
INSTANTIATE_TEST_SUITE_P(AllDirections, ProtocolMoveTest,
                         ::testing::Values(Direction::NORTH, Direction::SOUTH, Direction::EAST,
                                           Direction::WEST));

// ─────────────────────────────────────────────────────────────
// MOVE - raw byte verification
// ─────────────────────────────────────────────────────────────

// Verifies the exact binary format of the MOVE message
TEST_F(ProtocolTest, SendMoveCheckRawBytes) {
    Socket client_skt = Socket::from_fd(fds[0]);
    Socket peer = Socket::from_fd(fds[1]);

    ClientProtocol client(std::move(client_skt));

    client.send_command(MoveCmd{Direction::EAST});

    uint8_t opcode;
    ASSERT_EQ(peer.recvall(&opcode, 1), 1);
    EXPECT_EQ(opcode, static_cast<uint8_t>(OpCode::MOVE));

    uint8_t direction;
    ASSERT_EQ(peer.recvall(&direction, 1), 1);
    EXPECT_EQ(direction, static_cast<uint8_t>(Direction::EAST));
}

// ─────────────────────────────────────────────────────────────
// MOVE - unknown opcode throws
// ─────────────────────────────────────────────────────────────

// The server must throw std::runtime_error when it receives an opcode that
// does not exist. This simulates a buggy client or a protocol attack.
TEST_F(ProtocolTest, UnknownCommandOpcodeThrows) {
    Socket reader = Socket::from_fd(fds[0]);
    Socket writer = Socket::from_fd(fds[1]);

    ServerProtocol server(std::move(reader));

    uint8_t invalid_opcode = 0xFF;
    writer.sendall(&invalid_opcode, 1);

    EXPECT_THROW({ server.recv_command(); }, std::runtime_error);
}

// ─────────────────────────────────────────────────────────────
// LOGIN command
// ─────────────────────────────────────────────────────────────

// Verifies that username and password survive serialization/deserialization.
TEST_F(ProtocolTest, LoginRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    LoginCmd sent{"myuser", "mypassword"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<LoginCmd>(cmd));
    const auto& received = std::get<LoginCmd>(cmd);
    EXPECT_EQ(received.username, sent.username);
    EXPECT_EQ(received.password, sent.password);
}

// Empty strings are valid: the protocol encodes them as length=0 with no payload bytes.
TEST_F(ProtocolTest, LoginEmptyFieldsRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(LoginCmd{"", ""});

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<LoginCmd>(cmd));
    const auto& received = std::get<LoginCmd>(cmd);
    EXPECT_EQ(received.username, "");
    EXPECT_EQ(received.password, "");
}

// ─────────────────────────────────────────────────────────────
// CREATE_CHARACTER command
// ─────────────────────────────────────────────────────────────

/*
 * The parameter is std::pair<Race, PlayerClass>: all 4 race+class combinations
 * are tested. auto [race, class_] = GetParam()
 */
class ProtocolCreateCharacterTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<std::pair<Race, PlayerClass>> {};

TEST_P(ProtocolCreateCharacterTest, RoundtripRaceAndClass) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    auto [race, class_] = GetParam();
    CreateCharacterCmd sent{"hero", "pass", race, class_};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<CreateCharacterCmd>(cmd));
    const auto& received = std::get<CreateCharacterCmd>(cmd);
    EXPECT_EQ(received.username, sent.username);
    EXPECT_EQ(received.password, sent.password);
    EXPECT_EQ(received.race, race);
    EXPECT_EQ(received.player_class, class_);
}

// Generates one test per race+class combination.
INSTANTIATE_TEST_SUITE_P(RaceClassCombinations, ProtocolCreateCharacterTest,
                         ::testing::Values(std::make_pair(Race::HUMAN, PlayerClass::WARRIOR),
                                           std::make_pair(Race::ELF, PlayerClass::MAGE),
                                           std::make_pair(Race::DWARF, PlayerClass::CLERIC),
                                           std::make_pair(Race::GNOME, PlayerClass::PALADIN)));

// ─────────────────────────────────────────────────────────────
// LOGIN_OK event
// ─────────────────────────────────────────────────────────────

// Verifies all event fields, including the uint32 ones (experience, hp, mana, gold)
// that must be converted to big-endian on send and back to host order on receive.
TEST_F(ProtocolTest, LoginOkRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    LoginOkEvent sent{};
    sent.player_id = 42;
    sent.username = "hero";
    sent.race = Race::ELF;
    sent.player_class = PlayerClass::MAGE;
    sent.level = 5;
    sent.experience = 12000;
    sent.hp_current = 80;
    sent.hp_max = 100;
    sent.mana_current = 40;
    sent.mana_max = 50;
    sent.gold = 999;
    sent.pos = {300, 160};

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<LoginOkEvent>(ev));
    const auto& received = std::get<LoginOkEvent>(ev);
    EXPECT_EQ(received.player_id, sent.player_id);
    EXPECT_EQ(received.username, sent.username);
    EXPECT_EQ(received.race, sent.race);
    EXPECT_EQ(received.player_class, sent.player_class);
    EXPECT_EQ(received.level, sent.level);
    EXPECT_EQ(received.experience, sent.experience);
    EXPECT_EQ(received.hp_current, sent.hp_current);
    EXPECT_EQ(received.hp_max, sent.hp_max);
    EXPECT_EQ(received.mana_current, sent.mana_current);
    EXPECT_EQ(received.mana_max, sent.mana_max);
    EXPECT_EQ(received.gold, sent.gold);
    EXPECT_EQ(received.pos.x, sent.pos.x);
    EXPECT_EQ(received.pos.y, sent.pos.y);
}

// ─────────────────────────────────────────────────────────────
// LOGIN_ERROR event
// ─────────────────────────────────────────────────────────────

// Parameterized over all three LoginError enum values to ensure each error
// code survives serialization.
class ProtocolLoginErrorTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<LoginError> {};

TEST_P(ProtocolLoginErrorTest, RoundtripAllErrorCodes) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    LoginErrorEvent sent{GetParam(), "some error message"};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<LoginErrorEvent>(ev));
    const auto& received = std::get<LoginErrorEvent>(ev);
    EXPECT_EQ(received.error_code, sent.error_code);
    EXPECT_EQ(received.message, sent.message);
}

INSTANTIATE_TEST_SUITE_P(AllLoginErrors, ProtocolLoginErrorTest,
                         ::testing::Values(LoginError::INVALID_CREDENTIALS,
                                           LoginError::ALREADY_LOGGED_IN, LoginError::SERVER_FULL));

// ─────────────────────────────────────────────────────────────
// CHARACTER_CREATED event
// ─────────────────────────────────────────────────────────────

// CHARACTER_CREATED reuses the same payload as LOGIN_OK.
// CharacterCreatedEvent wraps a LoginOkEvent in its "data" field.
TEST_F(ProtocolTest, CharacterCreatedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    CharacterCreatedEvent sent{LoginOkEvent{1,
                                            "newchar",
                                            Race::DWARF,
                                            PlayerClass::PALADIN,
                                            1,
                                            0,
                                            0,
                                            100,
                                            100,
                                            50,
                                            50,
                                            0,
                                            {10, 20}}};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<CharacterCreatedEvent>(ev));
    const auto& received = std::get<CharacterCreatedEvent>(ev);
    EXPECT_EQ(received.data.player_id, sent.data.player_id);
    EXPECT_EQ(received.data.username, sent.data.username);
    EXPECT_EQ(received.data.race, sent.data.race);
    EXPECT_EQ(received.data.player_class, sent.data.player_class);
    EXPECT_EQ(received.data.level, sent.data.level);
    EXPECT_EQ(received.data.gold, sent.data.gold);
    EXPECT_EQ(received.data.pos.x, sent.data.pos.x);
    EXPECT_EQ(received.data.pos.y, sent.data.pos.y);
}

// ─────────────────────────────────────────────────────────────
// CHARACTER_ERROR event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, CharacterErrorRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    CharacterErrorEvent sent{CharacterError::USERNAME_TAKEN, "name already in use"};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<CharacterErrorEvent>(ev));
    const auto& received = std::get<CharacterErrorEvent>(ev);
    EXPECT_EQ(received.error_code, sent.error_code);
    EXPECT_EQ(received.message, sent.message);
}

// ─────────────────────────────────────────────────────────────
// ENTITY_SPAWN event
// ─────────────────────────────────────────────────────────────

// Player case: verifies all fields including name, race, class, and initial direction.
TEST_F(ProtocolTest, EntitySpawnPlayerRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    EntitySpawnEvent sent{};
    sent.entity_id = 7;
    sent.entity_type = EntityType::PLAYER;
    sent.entity_pos = {128, 256};
    sent.entity_dir = Direction::NORTH;
    sent.entity_name = "warrior";
    sent.entity_race = Race::HUMAN;
    sent.entity_class = PlayerClass::WARRIOR;

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<EntitySpawnEvent>(ev));
    const auto& received = std::get<EntitySpawnEvent>(ev);
    EXPECT_EQ(received.entity_id, sent.entity_id);
    EXPECT_EQ(received.entity_type, sent.entity_type);
    EXPECT_EQ(received.entity_pos.x, sent.entity_pos.x);
    EXPECT_EQ(received.entity_pos.y, sent.entity_pos.y);
    EXPECT_EQ(received.entity_dir, sent.entity_dir);
    EXPECT_EQ(received.entity_name, sent.entity_name);
    EXPECT_EQ(received.entity_race, sent.entity_race);
    EXPECT_EQ(received.entity_class, sent.entity_class);
}

// NPC case: verifies that entity_type=NPC serializes correctly
// (same format as PLAYER, different enum value).
TEST_F(ProtocolTest, EntitySpawnNpcRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    EntitySpawnEvent sent{};
    sent.entity_id = 100;
    sent.entity_type = EntityType::NPC;
    sent.entity_pos = {0, 0};
    sent.entity_dir = Direction::SOUTH;
    sent.entity_name = "goblin";
    sent.entity_race = Race::GNOME;
    sent.entity_class = PlayerClass::CLERIC;

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<EntitySpawnEvent>(ev));
    EXPECT_EQ(std::get<EntitySpawnEvent>(ev).entity_type, EntityType::NPC);
    EXPECT_EQ(std::get<EntitySpawnEvent>(ev).entity_name, "goblin");
}

// ─────────────────────────────────────────────────────────────
// ENTITY_DESPAWN event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, EntityDespawnRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    EntityDespawnEvent sent{.entity_id = 42};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<EntityDespawnEvent>(ev));
    EXPECT_EQ(std::get<EntityDespawnEvent>(ev).entity_id, sent.entity_id);
}

// ─────────────────────────────────────────────────────────────
// ENTITY_MOVE event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, EntityMoveRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    EntityMoveEvent sent{99, {512, 768}, Direction::WEST};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<EntityMoveEvent>(ev));
    const auto& received = std::get<EntityMoveEvent>(ev);
    EXPECT_EQ(received.entity_id, sent.entity_id);
    EXPECT_EQ(received.entity_pos.x, sent.entity_pos.x);
    EXPECT_EQ(received.entity_pos.y, sent.entity_pos.y);
    EXPECT_EQ(received.entity_dir, sent.entity_dir);
}

// ─────────────────────────────────────────────────────────────
// Unknown event opcode throws on the client
// ─────────────────────────────────────────────────────────────

// Symmetric to UnknownCommandOpcodeThrows: the client must throw if it receives
// a command opcode (0x01) where it expected an event opcode (0x80-0x9B).
TEST_F(ProtocolTest, UnknownEventOpcodeThrows) {
    Socket writer = Socket::from_fd(fds[0]);
    ClientProtocol client(Socket::from_fd(fds[1]));

    uint8_t invalid_opcode = 0x01;  // command opcode, not an event
    writer.sendall(&invalid_opcode, 1);

    EXPECT_THROW({ client.recv_event(); }, std::runtime_error);
}

// ─────────────────────────────────────────────────────────────
// Multiple messages in sequence
// ─────────────────────────────────────────────────────────────

// Verifies that messages are queued and dispatched in FIFO order.
TEST_F(ProtocolTest, MultipleCommandsInSequence) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(LoginCmd{"user1", "pass1"});
    client.send_command(MoveCmd{Direction::NORTH});
    client.send_command(MoveCmd{Direction::SOUTH});

    ClientCommand cmd1 = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<LoginCmd>(cmd1));
    EXPECT_EQ(std::get<LoginCmd>(cmd1).username, "user1");

    ClientCommand cmd2 = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<MoveCmd>(cmd2));
    EXPECT_EQ(std::get<MoveCmd>(cmd2).direction, Direction::NORTH);

    ClientCommand cmd3 = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<MoveCmd>(cmd3));
    EXPECT_EQ(std::get<MoveCmd>(cmd3).direction, Direction::SOUTH);
}

// Same concept for events: the client must dequeue them in the same order
// the server sent them.
TEST_F(ProtocolTest, MultipleEventsInSequence) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    server.send_event(LoginOkEvent{
            1, "hero", Race::HUMAN, PlayerClass::WARRIOR, 1, 0, 0, 100, 100, 50, 50, 0, {0, 0}});
    server.send_event(EntitySpawnEvent{
            .entity_id = 1,
            .entity_type = EntityType::PLAYER,
            .entity_pos = {0, 0},
            .entity_dir = Direction::SOUTH,
            .entity_name = "hero",
            .entity_race = Race::HUMAN,
            .entity_class = PlayerClass::WARRIOR,
            .clan_name = {},
    });
    server.send_event(EntityMoveEvent{1, {10, 0}, Direction::EAST});

    EXPECT_TRUE(std::holds_alternative<LoginOkEvent>(client.recv_event()));
    EXPECT_TRUE(std::holds_alternative<EntitySpawnEvent>(client.recv_event()));
    EXPECT_TRUE(std::holds_alternative<EntityMoveEvent>(client.recv_event()));
}

// ─────────────────────────────────────────────────────────────
// CLAN_NOTIFICATION event
// ─────────────────────────────────────────────────────────────

class ProtocolClanNotificationTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<ClanNotifType> {};

TEST_P(ProtocolClanNotificationTest, RoundtripAllTypes) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    ClanNotificationEvent sent{GetParam(), "player1", "TestClan"};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<ClanNotificationEvent>(ev));
    const auto& received = std::get<ClanNotificationEvent>(ev);
    EXPECT_EQ(received.type, sent.type);
    EXPECT_EQ(received.username, sent.username);
    EXPECT_EQ(received.clan_name, sent.clan_name);
}

INSTANTIATE_TEST_SUITE_P(AllClanNotifTypes, ProtocolClanNotificationTest,
                         ::testing::Values(ClanNotifType::MEMBER_ONLINE,
                                           ClanNotifType::MEMBER_OFFLINE,
                                           ClanNotifType::MEMBER_ATTACKED,
                                           ClanNotifType::JOIN_REQUEST,
                                           ClanNotifType::JOIN_ACCEPTED,
                                           ClanNotifType::JOIN_REJECTED, ClanNotifType::KICKED));

// ─────────────────────────────────────────────────────────────
// CLAN_UPDATE event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, ClanUpdateRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    ClanUpdateEvent sent;
    sent.clan_name = "TestClan";
    sent.members.push_back({"founder", true, true});
    sent.members.push_back({"member1", false, true});
    sent.members.push_back({"member2", false, false});

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<ClanUpdateEvent>(ev));
    const auto& received = std::get<ClanUpdateEvent>(ev);
    EXPECT_EQ(received.clan_name, sent.clan_name);
    ASSERT_EQ(received.members.size(), 3);
    EXPECT_EQ(received.members[0].username, "founder");
    EXPECT_EQ(received.members[0].is_founder, true);
    EXPECT_EQ(received.members[0].is_online, true);
    EXPECT_EQ(received.members[1].username, "member1");
    EXPECT_EQ(received.members[1].is_founder, false);
    EXPECT_EQ(received.members[1].is_online, true);
    EXPECT_EQ(received.members[2].username, "member2");
    EXPECT_EQ(received.members[2].is_founder, false);
    EXPECT_EQ(received.members[2].is_online, false);
}

// ─────────────────────────────────────────────────────────────
// ATTACK command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, AttackRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    AttackCmd sent{.target_id = 42};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<AttackCmd>(cmd));
    EXPECT_EQ(std::get<AttackCmd>(cmd).target_id, sent.target_id);
}

// ─────────────────────────────────────────────────────────────
// SEND_CHAT_MSG command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, SendChatMsgRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    SendChatMsgCmd sent{.text = "hola mundo"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<SendChatMsgCmd>(cmd));
    EXPECT_EQ(std::get<SendChatMsgCmd>(cmd).text, sent.text);
}

TEST_F(ProtocolTest, SendChatMsgEmptyTextRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(SendChatMsgCmd{.text = ""});

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<SendChatMsgCmd>(cmd));
    EXPECT_EQ(std::get<SendChatMsgCmd>(cmd).text, "");
}

// ─────────────────────────────────────────────────────────────
// Zero-field commands (opcode-only)
// ─────────────────────────────────────────────────────────────

class ZeroFieldCommandTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<ClientCommand> {};

TEST_P(ZeroFieldCommandTest, Roundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(GetParam());
    ClientCommand received = server.recv_command();
    EXPECT_EQ(received.index(), GetParam().index());
}

INSTANTIATE_TEST_SUITE_P(
        ZeroFieldCommands, ZeroFieldCommandTest,
        ::testing::Values(ClientCommand{MeditateCmd{}}, ClientCommand{ResurrectCmd{}},
                          ClientCommand{CheatInfiniteHpCmd{}},
                          ClientCommand{CheatInfiniteManaCmd{}}, ClientCommand{CheatDieCmd{}},
                          ClientCommand{CheatLevelUpCmd{}}, ClientCommand{CheatLevelDownCmd{}},
                          ClientCommand{NpcHealCmd{}}, ClientCommand{NpcListCmd{}},
                          ClientCommand{ClanReviewCmd{}}, ClientCommand{ClanLeaveCmd{}},
                          ClientCommand{CheatAddGoldCmd{}}, ClientCommand{CheatResetGoldCmd{}},
                          ClientCommand{CheatVelocityCmd{}}, ClientCommand{CheatReviveCmd{}},
                          ClientCommand{CheatFillInventoryCmd{}},
                          ClientCommand{CheatClearInventoryCmd{}},
                          ClientCommand{CheatResetManaCmd{}}));

// ─────────────────────────────────────────────────────────────
// DAMAGE_DEALT event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, DamageDealtRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    DamageDealtEvent sent{.target_id = 7, .damage = 42};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<DamageDealtEvent>(ev));
    const auto& received = std::get<DamageDealtEvent>(ev);
    EXPECT_EQ(received.target_id, sent.target_id);
    EXPECT_EQ(received.damage, sent.damage);
}

// ─────────────────────────────────────────────────────────────
// DAMAGE_RECEIVED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, DamageReceivedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    DamageReceivedEvent sent{
            .target_id = 3, .attacker_id = 5, .damage = 25, .hp_current = 75, .hp_max = 100};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<DamageReceivedEvent>(ev));
    const auto& received = std::get<DamageReceivedEvent>(ev);
    EXPECT_EQ(received.target_id, sent.target_id);
    EXPECT_EQ(received.attacker_id, sent.attacker_id);
    EXPECT_EQ(received.damage, sent.damage);
    EXPECT_EQ(received.hp_current, sent.hp_current);
    EXPECT_EQ(received.hp_max, sent.hp_max);
}

// ─────────────────────────────────────────────────────────────
// ENTITY_DIED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, EntityDiedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    EntityDiedEvent sent{.entity_id = 99};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<EntityDiedEvent>(ev));
    EXPECT_EQ(std::get<EntityDiedEvent>(ev).entity_id, sent.entity_id);
}

// ─────────────────────────────────────────────────────────────
// PLAYER_RESPAWNED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, PlayerRespawnedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    PlayerRespawnedEvent sent{.entity_id = 12, .hp_current = 80, .hp_max = 100};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<PlayerRespawnedEvent>(ev));
    const auto& received = std::get<PlayerRespawnedEvent>(ev);
    EXPECT_EQ(received.entity_id, sent.entity_id);
    EXPECT_EQ(received.hp_current, sent.hp_current);
    EXPECT_EQ(received.hp_max, sent.hp_max);
}

// ─────────────────────────────────────────────────────────────
// MEDITATION_START / MEDITATION_STOP events
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, MeditationStartRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    server.send_event(MeditationStartEvent{});

    ServerEvent ev = client.recv_event();
    EXPECT_TRUE(std::holds_alternative<MeditationStartEvent>(ev));
}

TEST_F(ProtocolTest, MeditationStopRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    server.send_event(MeditationStopEvent{});

    ServerEvent ev = client.recv_event();
    EXPECT_TRUE(std::holds_alternative<MeditationStopEvent>(ev));
}

// ─────────────────────────────────────────────────────────────
// CHAT_MSG event - all types
// ─────────────────────────────────────────────────────────────

class ProtocolChatMsgTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<ChatMsgType> {};

TEST_P(ProtocolChatMsgTest, RoundtripAllTypes) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    ChatMsgEvent sent{GetParam(), "sender", "hello", 2, 1};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<ChatMsgEvent>(ev));
    const auto& received = std::get<ChatMsgEvent>(ev);
    EXPECT_EQ(received.type, sent.type);
    EXPECT_EQ(received.sender_name, sent.sender_name);
    EXPECT_EQ(received.message, sent.message);
    EXPECT_EQ(received.recipient_id, sent.recipient_id);
    EXPECT_EQ(received.sender_id, sent.sender_id);
}

INSTANTIATE_TEST_SUITE_P(AllChatTypes, ProtocolChatMsgTest,
                         ::testing::Values(ChatMsgType::SAY, ChatMsgType::PRIVATE,
                                           ChatMsgType::CLAN, ChatMsgType::SYSTEM));

// ─────────────────────────────────────────────────────────────
// CAST_SPELL command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, CastSpellRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    CastSpellCmd sent{.target_id = 123};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<CastSpellCmd>(cmd));
    EXPECT_EQ(std::get<CastSpellCmd>(cmd).target_id, sent.target_id);
}

// ─────────────────────────────────────────────────────────────
// PICKUP_ITEM command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, PickupItemRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    PickupItemCmd sent{.item_name = "sword"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<PickupItemCmd>(cmd));
    EXPECT_EQ(std::get<PickupItemCmd>(cmd).item_name, sent.item_name);
}

TEST_F(ProtocolTest, PickupItemEmptyNameRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(PickupItemCmd{.item_name = ""});

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<PickupItemCmd>(cmd));
    EXPECT_EQ(std::get<PickupItemCmd>(cmd).item_name, "");
}

// ─────────────────────────────────────────────────────────────
// DROP_ITEM command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, DropItemRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    DropItemCmd sent{.item_name = "shield"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<DropItemCmd>(cmd));
    EXPECT_EQ(std::get<DropItemCmd>(cmd).item_name, sent.item_name);
}

// ─────────────────────────────────────────────────────────────
// EQUIP_ITEM command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, EquipItemRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    EquipItemCmd sent{.slot_index = 3};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<EquipItemCmd>(cmd));
    EXPECT_EQ(std::get<EquipItemCmd>(cmd).slot_index, sent.slot_index);
}

// ─────────────────────────────────────────────────────────────
// UNEQUIP_ITEM command
// ─────────────────────────────────────────────────────────────

class ProtocolUnequipItemTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<EquipSlot> {};

TEST_P(ProtocolUnequipItemTest, RoundtripAllSlots) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    EquipSlot slot = GetParam();
    client.send_command(UnequipItemCmd{slot});

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<UnequipItemCmd>(cmd));
    EXPECT_EQ(std::get<UnequipItemCmd>(cmd).slot, slot);
}

INSTANTIATE_TEST_SUITE_P(AllEquipSlots, ProtocolUnequipItemTest,
                         ::testing::Values(EquipSlot::WEAPON, EquipSlot::ARMOR, EquipSlot::HELMET,
                                           EquipSlot::SHIELD, EquipSlot::CONSUMABLE,
                                           EquipSlot::NONE));

// ─────────────────────────────────────────────────────────────
// NPC_BUY command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, NpcBuyRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    NpcBuyCmd sent{.item_name = "health_potion"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<NpcBuyCmd>(cmd));
    EXPECT_EQ(std::get<NpcBuyCmd>(cmd).item_name, sent.item_name);
}

// ─────────────────────────────────────────────────────────────
// NPC_SELL command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, NpcSellRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    NpcSellCmd sent{.item_name = "iron_helmet"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<NpcSellCmd>(cmd));
    EXPECT_EQ(std::get<NpcSellCmd>(cmd).item_name, sent.item_name);
}

// ─────────────────────────────────────────────────────────────
// BANK_DEPOSIT command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, BankDepositGoldRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    BankDepositCmd sent{.is_gold = true, .item_name = "", .gold_amount = 500};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<BankDepositCmd>(cmd));
    const auto& received = std::get<BankDepositCmd>(cmd);
    EXPECT_EQ(received.is_gold, sent.is_gold);
    EXPECT_EQ(received.gold_amount, sent.gold_amount);
}

TEST_F(ProtocolTest, BankDepositItemRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    BankDepositCmd sent{.is_gold = false, .item_name = "leather_armor", .gold_amount = 0};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<BankDepositCmd>(cmd));
    const auto& received = std::get<BankDepositCmd>(cmd);
    EXPECT_EQ(received.is_gold, sent.is_gold);
    EXPECT_EQ(received.item_name, sent.item_name);
}

// ─────────────────────────────────────────────────────────────
// BANK_WITHDRAW command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, BankWithdrawGoldRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    BankWithdrawCmd sent{.is_gold = true, .item_name = "", .gold_amount = 300};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<BankWithdrawCmd>(cmd));
    const auto& received = std::get<BankWithdrawCmd>(cmd);
    EXPECT_EQ(received.is_gold, sent.is_gold);
    EXPECT_EQ(received.gold_amount, sent.gold_amount);
}

TEST_F(ProtocolTest, BankWithdrawItemRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    BankWithdrawCmd sent{.is_gold = false, .item_name = "plate_armor", .gold_amount = 0};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<BankWithdrawCmd>(cmd));
    const auto& received = std::get<BankWithdrawCmd>(cmd);
    EXPECT_EQ(received.is_gold, sent.is_gold);
    EXPECT_EQ(received.item_name, sent.item_name);
}

// ─────────────────────────────────────────────────────────────
// Clan string commands (parameterized over all clan commands with a string field)
// ─────────────────────────────────────────────────────────────

using ClanStringCmdParam = std::pair<ClientCommand, std::string>;

class ProtocolClanStringCmdTest:
        public ProtocolTest,
        public ::testing::WithParamInterface<ClanStringCmdParam> {};

TEST_P(ProtocolClanStringCmdTest, Roundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(GetParam().first);

    ClientCommand received = server.recv_command();
    EXPECT_EQ(received.index(), GetParam().first.index());
}

INSTANTIATE_TEST_SUITE_P(
        ClanStringCommands, ProtocolClanStringCmdTest,
        ::testing::Values(ClanStringCmdParam{ClanFoundCmd{"MyClan"}, "MyClan"},
                          ClanStringCmdParam{ClanJoinRequestCmd{"MyClan"}, "MyClan"},
                          ClanStringCmdParam{ClanAcceptCmd{"player1"}, "player1"},
                          ClanStringCmdParam{ClanRejectCmd{"player1"}, "player1"},
                          ClanStringCmdParam{ClanBanCmd{"badplayer"}, "badplayer"},
                          ClanStringCmdParam{ClanUnbanCmd{"badplayer"}, "badplayer"},
                          ClanStringCmdParam{ClanKickCmd{"member1"}, "member1"}));

// ─────────────────────────────────────────────────────────────
// CHANGE_MAP command
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, ChangeMapRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    ChangeMapCmd sent{.prop_name = "map001"};
    client.send_command(sent);

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<ChangeMapCmd>(cmd));
    EXPECT_EQ(std::get<ChangeMapCmd>(cmd).prop_name, sent.prop_name);
}

// ─────────────────────────────────────────────────────────────
// ATTACK_DODGED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, AttackDodgedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    AttackDodgedEvent sent{.player_id = 7};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<AttackDodgedEvent>(ev));
    EXPECT_EQ(std::get<AttackDodgedEvent>(ev).player_id, sent.player_id);
}

// ─────────────────────────────────────────────────────────────
// GOLD_UPDATE event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, GoldUpdateRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    GoldUpdateEvent sent{.gold = 12345};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<GoldUpdateEvent>(ev));
    EXPECT_EQ(std::get<GoldUpdateEvent>(ev).gold, sent.gold);
}

// ─────────────────────────────────────────────────────────────
// HEAL_RECEIVED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, HealReceivedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    HealReceivedEvent sent{.player_id = 1, .hp_current = 90, .mana_current = 45};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<HealReceivedEvent>(ev));
    const auto& received = std::get<HealReceivedEvent>(ev);
    EXPECT_EQ(received.player_id, sent.player_id);
    EXPECT_EQ(received.hp_current, sent.hp_current);
    EXPECT_EQ(received.mana_current, sent.mana_current);
}

// ─────────────────────────────────────────────────────────────
// SPELL_EFFECT event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, SpellEffectRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    SpellEffectEvent sent{.target_id = 42, .effect_type = 0};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<SpellEffectEvent>(ev));
    const auto& received = std::get<SpellEffectEvent>(ev);
    EXPECT_EQ(received.target_id, sent.target_id);
    EXPECT_EQ(received.effect_type, sent.effect_type);
}

// ─────────────────────────────────────────────────────────────
// MAP_TRANSITION event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, MapTransitionRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    MapTransitionEvent sent{.map_name = "dungeon_01", .pos_x = 100, .pos_y = 200};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<MapTransitionEvent>(ev));
    const auto& received = std::get<MapTransitionEvent>(ev);
    EXPECT_EQ(received.map_name, sent.map_name);
    EXPECT_EQ(received.pos_x, sent.pos_x);
    EXPECT_EQ(received.pos_y, sent.pos_y);
}

// ─────────────────────────────────────────────────────────────
// PLAYER_STATS event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, PlayerStatsRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    PlayerStatsEvent sent{};
    sent.level = 10;
    sent.experience = 5000;
    sent.exp_to_next = 10000;
    sent.hp_current = 75;
    sent.hp_max = 100;
    sent.mana_current = 30;
    sent.mana_max = 50;
    sent.crit_chance = 15;
    sent.damage_min = 8;
    sent.damage_max = 20;
    sent.defense_min = 3;
    sent.defense_max = 10;
    sent.dodge_chance = 5;
    sent.strength = 12;
    sent.agility = 9;

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<PlayerStatsEvent>(ev));
    const auto& r = std::get<PlayerStatsEvent>(ev);
    EXPECT_EQ(r.level, sent.level);
    EXPECT_EQ(r.experience, sent.experience);
    EXPECT_EQ(r.exp_to_next, sent.exp_to_next);
    EXPECT_EQ(r.hp_current, sent.hp_current);
    EXPECT_EQ(r.hp_max, sent.hp_max);
    EXPECT_EQ(r.mana_current, sent.mana_current);
    EXPECT_EQ(r.mana_max, sent.mana_max);
    EXPECT_EQ(r.crit_chance, sent.crit_chance);
    EXPECT_EQ(r.damage_min, sent.damage_min);
    EXPECT_EQ(r.damage_max, sent.damage_max);
    EXPECT_EQ(r.defense_min, sent.defense_min);
    EXPECT_EQ(r.defense_max, sent.defense_max);
    EXPECT_EQ(r.dodge_chance, sent.dodge_chance);
    EXPECT_EQ(r.strength, sent.strength);
    EXPECT_EQ(r.agility, sent.agility);
}

// ─────────────────────────────────────────────────────────────
// INVENTORY_UPDATE event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, InventoryUpdateEmptyRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    InventoryUpdateEvent sent{std::vector<InventorySlot>{}};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<InventoryUpdateEvent>(ev));
    EXPECT_TRUE(std::get<InventoryUpdateEvent>(ev).slots.empty());
}

TEST_F(ProtocolTest, InventoryUpdateWithItemsRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    std::vector<InventorySlot> slots;
    slots.push_back({0, ItemType::SWORD, "bronze_sword"});
    slots.push_back({1, ItemType::HEALTH_POTION, "health_potion"});
    InventoryUpdateEvent sent{std::move(slots)};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<InventoryUpdateEvent>(ev));
    const auto& received = std::get<InventoryUpdateEvent>(ev);
    ASSERT_EQ(received.slots.size(), 2);
    EXPECT_EQ(received.slots[0].slot_index, 0);
    EXPECT_EQ(received.slots[0].item_type, ItemType::SWORD);
    EXPECT_EQ(received.slots[0].item_name, "bronze_sword");
    EXPECT_EQ(received.slots[1].slot_index, 1);
    EXPECT_EQ(received.slots[1].item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(received.slots[1].item_name, "health_potion");
}

// ─────────────────────────────────────────────────────────────
// EQUIP_UPDATE event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, EquipUpdateRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    EquipUpdateEvent sent{};
    sent.entity_id = 1;
    sent.weapon = {0, ItemType::SWORD, "iron_sword"};
    sent.armor = {1, ItemType::PLATE_ARMOR, "plate_armor"};
    sent.helmet = {2, ItemType::IRON_HELMET, "iron_helmet"};
    sent.shield = {3, ItemType::IRON_SHIELD, "iron_shield"};

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<EquipUpdateEvent>(ev));
    const auto& received = std::get<EquipUpdateEvent>(ev);
    EXPECT_EQ(received.entity_id, sent.entity_id);
    EXPECT_EQ(received.weapon.item_type, sent.weapon.item_type);
    EXPECT_EQ(received.weapon.item_name, sent.weapon.item_name);
    EXPECT_EQ(received.armor.item_type, sent.armor.item_type);
    EXPECT_EQ(received.armor.item_name, sent.armor.item_name);
    EXPECT_EQ(received.helmet.item_type, sent.helmet.item_type);
    EXPECT_EQ(received.helmet.item_name, sent.helmet.item_name);
    EXPECT_EQ(received.shield.item_type, sent.shield.item_type);
    EXPECT_EQ(received.shield.item_name, sent.shield.item_name);
}

// ─────────────────────────────────────────────────────────────
// NPC_ITEM_LIST event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, NpcItemListEmptyRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    NpcItemListEvent sent{std::vector<NpcItemEntry>{}};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<NpcItemListEvent>(ev));
    EXPECT_TRUE(std::get<NpcItemListEvent>(ev).items.empty());
}

TEST_F(ProtocolTest, NpcItemListWithItemsRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    std::vector<NpcItemEntry> items;
    items.push_back({"iron_sword", ItemType::SWORD, 1, 100});
    items.push_back({"health_potion", ItemType::HEALTH_POTION, 2, 50});
    NpcItemListEvent sent{std::move(items)};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<NpcItemListEvent>(ev));
    const auto& received = std::get<NpcItemListEvent>(ev);
    ASSERT_EQ(received.items.size(), 2);
    EXPECT_EQ(received.items[0].item_name, "iron_sword");
    EXPECT_EQ(received.items[0].item_type, ItemType::SWORD);
    EXPECT_EQ(received.items[0].sprite_id, 1);
    EXPECT_EQ(received.items[0].price, 100);
    EXPECT_EQ(received.items[1].item_name, "health_potion");
    EXPECT_EQ(received.items[1].item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(received.items[1].sprite_id, 2);
    EXPECT_EQ(received.items[1].price, 50);
}

// ─────────────────────────────────────────────────────────────
// ITEM_DROPPED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, ItemDroppedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    ItemDroppedEvent sent{};
    sent.pos = {10, 20};
    sent.item_type = ItemType::GOLD_DROP;
    sent.item_name = "gold";
    sent.amount = 50;

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<ItemDroppedEvent>(ev));
    const auto& received = std::get<ItemDroppedEvent>(ev);
    EXPECT_EQ(received.pos.x, sent.pos.x);
    EXPECT_EQ(received.pos.y, sent.pos.y);
    EXPECT_EQ(received.item_type, sent.item_type);
    EXPECT_EQ(received.item_name, sent.item_name);
    EXPECT_EQ(received.amount, sent.amount);
}

// ─────────────────────────────────────────────────────────────
// ITEM_PICKED event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, ItemPickedRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    ItemPickedEvent sent{};
    sent.pos = {5, 15};
    sent.item_name = "gold";
    sent.amount = 30;

    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<ItemPickedEvent>(ev));
    const auto& received = std::get<ItemPickedEvent>(ev);
    EXPECT_EQ(received.pos.x, sent.pos.x);
    EXPECT_EQ(received.pos.y, sent.pos.y);
    EXPECT_EQ(received.item_name, sent.item_name);
    EXPECT_EQ(received.amount, sent.amount);
}

// ─────────────────────────────────────────────────────────────
// BANK_UPDATE event
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, BankUpdateEmptyRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    BankUpdateEvent sent{std::vector<InventorySlot>{}, 0};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<BankUpdateEvent>(ev));
    const auto& received = std::get<BankUpdateEvent>(ev);
    EXPECT_TRUE(received.slots.empty());
    EXPECT_EQ(received.gold, 0);
}

TEST_F(ProtocolTest, BankUpdateWithItemsRoundtrip) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    std::vector<InventorySlot> slots;
    slots.push_back({0, ItemType::ASH_STAFF, "ash_staff"});
    BankUpdateEvent sent{std::move(slots), 999};
    server.send_event(sent);

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<BankUpdateEvent>(ev));
    const auto& received = std::get<BankUpdateEvent>(ev);
    ASSERT_EQ(received.slots.size(), 1);
    EXPECT_EQ(received.slots[0].slot_index, 0);
    EXPECT_EQ(received.slots[0].item_type, ItemType::ASH_STAFF);
    EXPECT_EQ(received.slots[0].item_name, "ash_staff");
    EXPECT_EQ(received.gold, 999);
}

// ─────────────────────────────────────────────────────────────
// MAP_DATA / REQUEST_MAP_DATA (descarga de niveles por red)
// ─────────────────────────────────────────────────────────────

TEST_F(ProtocolTest, RequestMapDataRoundtrip) {
    ClientProtocol client(Socket::from_fd(fds[0]));
    ServerProtocol server(Socket::from_fd(fds[1]));

    client.send_command(RequestMapDataCmd{"dungeon"});

    ClientCommand cmd = server.recv_command();
    ASSERT_TRUE(std::holds_alternative<RequestMapDataCmd>(cmd));
    EXPECT_EQ(std::get<RequestMapDataCmd>(cmd).map_name, "dungeon");
}

TEST_F(ProtocolTest, MapDataRoundtripPreservesDictionaryAndGrids) {
    ServerProtocol server(Socket::from_fd(fds[0]));
    ClientProtocol client(Socket::from_fd(fds[1]));

    // Mapa 3x3 armado a mano, con diccionario de 2 tiles y 2 props.
    MapLevelData sent;
    sent.map_name = "city";
    sent.map_type = MapType::CITY;
    sent.tile_size = 128;
    sent.rows = 3;
    sent.cols = 3;
    sent.tile_id_table = {"grass", "wall"};
    sent.tile_grid = {
            {0, 0, 0},
            {0, 1, 0},
            {0, 0, 0},
    };
    sent.prop_id_table = {"tree", "door"};
    sent.props = {
            {0, 0, 2, false},  // tree en (0,2)
            {1, 2, 0, true},   // door en (2,0), transición
    };
    sent.walkable = {
            {true, true, false},
            {true, false, true},
            {true, true, true},
    };
    sent.mob_spawn_zones = {
            {false, false, false},
            {false, true, false},
            {false, false, false},
    };

    server.send_event(MapDataEvent{sent});

    ServerEvent ev = client.recv_event();
    ASSERT_TRUE(std::holds_alternative<MapDataEvent>(ev));
    const MapLevelData& got = std::get<MapDataEvent>(ev).data;

    EXPECT_EQ(got.map_name, "city");
    EXPECT_EQ(got.map_type, MapType::CITY);
    EXPECT_EQ(got.tile_size, 128);
    EXPECT_EQ(got.rows, 3);
    EXPECT_EQ(got.cols, 3);

    // El diccionario y los índices resuelven al mismo string del otro lado.
    ASSERT_EQ(got.tile_id_table, sent.tile_id_table);
    ASSERT_EQ(got.tile_grid, sent.tile_grid);
    EXPECT_EQ(got.tile_id_table[got.tile_grid[1][1]], "wall");

    ASSERT_EQ(got.prop_id_table, sent.prop_id_table);
    ASSERT_EQ(got.props.size(), 2u);
    EXPECT_EQ(got.prop_id_table[got.props[0].prop_id_index], "tree");
    EXPECT_FALSE(got.props[0].is_transition);
    EXPECT_EQ(got.prop_id_table[got.props[1].prop_id_index], "door");
    EXPECT_TRUE(got.props[1].is_transition);
    EXPECT_EQ(got.props[1].row, 2);
    EXPECT_EQ(got.props[1].col, 0);

    EXPECT_EQ(got.walkable, sent.walkable);
    EXPECT_EQ(got.mob_spawn_zones, sent.mob_spawn_zones);
}
