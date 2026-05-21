#include <sys/socket.h>
#include <unistd.h>

#include "client/network/client_protocol.h"
#include "common/messages.h"
#include "common/queue.h"
#include "common/socket.h"
#include "gtest/gtest.h"
#include "server/client_handler.h"
#include "server/player_command.h"

/*
 * ClientHandlerTest sets up a socketpair:
 *   fds[0] is given to ClientHandler (server side)
 *   fds[1] is kept as peer to simulate a client sending/receiving
 */
class ClientHandlerTest: public ::testing::Test {
protected:
    int fds[2];
    Queue<PlayerCommand> input_queue;

    void SetUp() override { ASSERT_NE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), -1); }

    void TearDown() override {
        if (fds[0] != -1)
            ::close(fds[0]);
        if (fds[1] != -1)
            ::close(fds[1]);
    }
};

// ─────────────────────────────────────────────────────────────
// Receiver thread: command arrives from the peer
// ─────────────────────────────────────────────────────────────

// The Receiver thread should push commands from the socket into the
// input_queue so the game loop can read them.
TEST_F(ClientHandlerTest, ReceiverPushesCommandToInputQueue) {
    ClientHandler handler(1, Socket::from_fd(fds[0]), input_queue);
    handler.start();

    // Simulate the client sending a MoveCmd via the peer socket.
    ClientProtocol peer(Socket::from_fd(fds[1]));
    peer.send_command(MoveCmd{Direction::NORTH});

    PlayerCommand pc = input_queue.pop();

    EXPECT_EQ(pc.player_id, 1);
    ASSERT_TRUE(std::holds_alternative<MoveCmd>(pc.cmd));
    EXPECT_EQ(std::get<MoveCmd>(pc.cmd).direction, Direction::NORTH);

    handler.stop();
    handler.join();
}

// ─────────────────────────────────────────────────────────────
// Sender thread: event queued on handler reaches the peer
// ─────────────────────────────────────────────────────────────

// The Sender thread should dequeue ServerEvents and write them to the socket.
TEST_F(ClientHandlerTest, SenderDeliversEventToPeer) {
    ClientHandler handler(2, Socket::from_fd(fds[0]), input_queue);
    handler.start();

    EntityMoveEvent ev{42, {100, 200}, Direction::EAST};
    handler.push_event(ev);

    ClientProtocol peer(Socket::from_fd(fds[1]));
    ServerEvent received = peer.recv_event();

    ASSERT_TRUE(std::holds_alternative<EntityMoveEvent>(received));
    const auto& got = std::get<EntityMoveEvent>(received);
    EXPECT_EQ(got.entity_id, ev.entity_id);
    EXPECT_EQ(got.entity_pos.x, ev.entity_pos.x);
    EXPECT_EQ(got.entity_pos.y, ev.entity_pos.y);
    EXPECT_EQ(got.entity_dir, ev.entity_dir);

    handler.stop();
    handler.join();
}

// ─────────────────────────────────────────────────────────────
// is_alive after start and after stop+join
// ─────────────────────────────────────────────────────────────

TEST_F(ClientHandlerTest, IsAliveAfterStartThenFalseAfterJoin) {
    ClientHandler handler(3, Socket::from_fd(fds[0]), input_queue);
    handler.start();

    EXPECT_TRUE(handler.is_alive());

    handler.stop();
    handler.join();

    EXPECT_FALSE(handler.is_alive());
}

// ─────────────────────────────────────────────────────────────
// Receiver detects peer disconnect: is_alive becomes false
// ─────────────────────────────────────────────────────────────

// When the peer closes its end of the socket, recv_command() on the server
// side should fail, the Receiver thread should exit, and is_alive() must
// return false.
TEST_F(ClientHandlerTest, IsAliveDropsWhenPeerDisconnects) {
    ClientHandler handler(4, Socket::from_fd(fds[0]), input_queue);
    handler.start();

    // Close the peer side to simulate a client disconnect.
    ::close(fds[1]);
    fds[1] = -1;  // prevent double close in TearDown

    // Spin until the Receiver thread notices.
    for (int i = 0; i < 200 && handler.is_alive(); ++i) {
        usleep(5000);
    }

    EXPECT_FALSE(handler.is_alive());

    handler.stop();
    handler.join();
}
