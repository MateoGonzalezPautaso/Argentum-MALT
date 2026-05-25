#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "../../common/queue.h"
#include "../../common/socket.h"
#include "../../common/thread.h"

#include "client_list_monitor.h"
#include "../game/player_command.h"

// Accepts incoming connections in a loop and registers each one in the monitor.
class Acceptor: public Thread {
    Socket& listener;
    Queue<PlayerCommand>& input_queue;
    ClientListMonitor& monitor;

public:
    Acceptor(Socket& listener, Queue<PlayerCommand>& input_queue, ClientListMonitor& monitor);

    void run() override;
    void stop() override;
};

#endif  // ACCEPTOR_H
