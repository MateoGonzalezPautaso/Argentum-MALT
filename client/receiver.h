#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "client_protocol.h"

class Receiver: public Thread {
private:
    ClientProtocol& protocol;
    Queue<ServerEvent>& queue;

public:
    Receiver(ClientProtocol& protocol, Queue<ServerEvent>& queue);
    void run() override;
};

#endif  // CLIENT_RECEIVER_H
