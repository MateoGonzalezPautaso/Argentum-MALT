#include "receiver.h"

#include <utility>

Receiver::Receiver(uint16_t player_id, ServerProtocol& protocol, Queue<PlayerCommand>& input_queue):
        player_id(player_id), protocol(protocol), input_queue(input_queue) {}

void Receiver::run() {
    try {
        while (should_keep_running()) {
            ClientCommand cmd = protocol.recv_command();
            input_queue.push(PlayerCommand{player_id, std::move(cmd)});
        }
    } catch (const ClosedQueue&) {
        // Input queue closed (server shutting down).
    } catch (const std::exception&) {
        // Connection dropped or recv failed.
    }
}
