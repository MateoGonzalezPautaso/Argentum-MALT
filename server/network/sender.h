#ifndef SENDER_H
#define SENDER_H

#include "../../common/messages.h"
#include "../../common/queue.h"
#include "../../common/thread.h"

#include "server_protocol.h"

// Drains each client output queue and sends each event to the client.
class Sender: public Thread {
    ServerProtocol& protocol;
    Queue<ServerEvent>& queue;

public:
    Sender(ServerProtocol& protocol, Queue<ServerEvent>& queue);
    void run() override;
};

#endif  // SENDER_H
