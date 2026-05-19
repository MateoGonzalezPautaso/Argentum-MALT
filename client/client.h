#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/socket.h"

#include "client_protocol.h"
#include "config.h"
#include "engine.h"
#include "receiver.h"
#include "sender.h"

// Client owns everything needed for a session
class Client {
private:
    ClientConfig config;
    Socket skt;
    ClientProtocol protocol;
    Queue<ClientCommand> command_queue;
    Queue<ServerEvent> event_queue;
    ClientEngine engine;
    Sender sender;
    Receiver receiver;

    // Reads credentials from stdin, sends LoginCmd, and blocks until the
    // server replies. Returns the LoginOkEvent on success, throws on error.
    LoginOkEvent do_login();

    bool run_menu();
    void game_loop();

    // Closes the command queue (unblocks Sender), then shuts down the socket
    // (unblocks Receiver), then joins both threads.
    void shutdown();

public:
    explicit Client(const ClientConfig& config);

    void run();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
};

#endif  // CLIENT_CLIENT_H
