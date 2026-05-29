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
    server.send_event(EntitySpawnEvent{1,
                                       EntityType::PLAYER,
                                       {0, 0},
                                       Direction::SOUTH,
                                       "hero",
                                       Race::HUMAN,
                                       PlayerClass::WARRIOR});
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
                                           ClanNotifType::JOIN_REJECTED,
                                           ClanNotifType::KICKED));

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

class ZeroFieldCommandTest: public ProtocolTest,
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
                          ClientCommand{CheatInfiniteHpCmd{}}, ClientCommand{CheatInfiniteManaCmd{}},
                          ClientCommand{CheatDieCmd{}}, ClientCommand{CheatLevelUpCmd{}},
                          ClientCommand{CheatLevelDownCmd{}}));

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

    DamageReceivedEvent sent{.target_id = 3, .attacker_id = 5, .damage = 25,
                              .hp_current = 75, .hp_max = 100};
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

class ProtocolChatMsgTest: public ProtocolTest,
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
