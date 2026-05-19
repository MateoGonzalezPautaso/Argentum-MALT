#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/thread.h"

#include "client_protocol.h"

// Receiver runs on its own thread, blocking on incoming server events and
// pushing each ServerEvent into the queue for the main thread to consume.
class Receiver: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ServerEvent>& queue;

public:
    Receiver(ClientProtocol& protocol, Queue<ServerEvent>& queue);
    void run() override;
};

#endif  // CLIENT_RECEIVER_H
