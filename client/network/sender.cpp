#include "sender.h"

Sender::Sender(ClientProtocol& protocol, Queue<ClientCommand>& queue):
        protocol(protocol), queue(queue) {}

void Sender::run() {
    try {
        while (should_keep_running()) {
            // Blocking pop: sleeps until a command is available or the queue is closed.
            ClientCommand cmd = queue.pop();
            protocol.send_command(cmd);
        }
    } catch (const ClosedQueue&) {
        // Normal shutdown: main thread closed the queue.
    } catch (const std::exception&) {}
}
