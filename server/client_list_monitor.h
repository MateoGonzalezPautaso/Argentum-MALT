#ifndef CLIENT_LIST_MONITOR_H
#define CLIENT_LIST_MONITOR_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/socket.h"

#include "client_handler.h"
#include "player_command.h"

class ClientListMonitor {
    std::mutex mtx;
    std::map<uint16_t, std::unique_ptr<ClientHandler>> clients;
    uint16_t next_id = 0;

public:
    ClientListMonitor() = default;

    // Adds a new client, starts its threads and returns the assigned player_id
    uint16_t add(Socket&& skt, Queue<PlayerCommand>& input_queue);

    // Sends an event to every connected client
    void broadcast(const ServerEvent& event);

    // Joins and removes clients whose threads have already exited.
    void clean_dead();

    // Stops and joins all clients (used on server shutdown).
    void stop_all();

    ClientListMonitor(const ClientListMonitor&) = delete;
    ClientListMonitor& operator=(const ClientListMonitor&) = delete;
};

#endif  // CLIENT_LIST_MONITOR_H
