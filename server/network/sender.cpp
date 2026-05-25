#include "sender.h"

Sender::Sender(ServerProtocol& protocol, Queue<ServerEvent>& queue):
        protocol(protocol), queue(queue) {}

void Sender::run() {
    try {
        while (should_keep_running()) {
            ServerEvent ev = queue.pop();
            protocol.send_event(ev);
        }
    } catch (const ClosedQueue&) {
        // Normal shutdown, ClientHandler closed the output queue.
    } catch (const std::exception&) {
        // Send failed (client disconnected).
    }
}
