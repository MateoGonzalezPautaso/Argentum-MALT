#ifndef RECEIVER_H
#define RECEIVER_H

#include <cstdint>

#include "../../common/messages.h"
#include "../../common/queue.h"
#include "../../common/thread.h"
#include "../game/player_command.h"

#include "server_protocol.h"

// Blocks on recv_command() and pushes each input_queue command (tagged with
// player_id) into the shared queue for the game loop
class Receiver: public Thread {
    uint16_t player_id;
    ServerProtocol& protocol;
    Queue<PlayerCommand>& input_queue;

public:
    Receiver(uint16_t player_id, ServerProtocol& protocol, Queue<PlayerCommand>& input_queue);
    void run() override;
};

#endif  // RECEIVER_H
