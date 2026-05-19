#ifndef CLIENT_SENDER_H
#define CLIENT_SENDER_H

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/thread.h"

#include "client_protocol.h"

// Sender runs on its own thread, draining the command queue and forwarding
// each ClientCommand to the server via ClientProtocol.
class Sender: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ClientCommand>& queue;

public:
    Sender(ClientProtocol& protocol, Queue<ClientCommand>& queue);
    void run() override;
};

#endif  // CLIENT_SENDER_H
