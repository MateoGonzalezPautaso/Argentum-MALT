#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <cstdint>

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/socket.h"

#include "player_command.h"
#include "receiver.h"
#include "sender.h"
#include "server_protocol.h"

// Owns the connection to one client and its two I/O threads.
class ClientHandler {
    uint16_t player_id;
    ServerProtocol protocol;
    Queue<ServerEvent> output_queue;
    Sender sender;
    Receiver receiver;

public:
    ClientHandler(uint16_t player_id, Socket&& skt, Queue<PlayerCommand>& input_queue);

    void start();

    // Push an event into this client's output queue.
    void push_event(const ServerEvent& ev);

    // Signal both threads to stop (non-blocking).
    void stop();

    // Wait for both threads to finish (must call stop() first).
    void join();

    // Returns false when either thread has exited (connection lost or shutdown).
    bool is_alive() const;

    ClientHandler(const ClientHandler&) = delete;
    ClientHandler& operator=(const ClientHandler&) = delete;
};

#endif  // CLIENT_HANDLER_H
