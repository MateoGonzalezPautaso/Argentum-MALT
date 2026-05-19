#include "receiver.h"

Receiver::Receiver(ClientProtocol& protocol, Queue<ServerEvent>& queue):
        protocol(protocol), queue(queue) {}

void Receiver::run() {
    try {
        while (should_keep_running()) {
            // Blocking recv: sleeps until a server event arrives or the socket closes.
            ServerEvent ev = protocol.recv_event();
            queue.push(ev);
        }
    } catch (const ClosedQueue&) {
        // Normal shutdown: main thread closed the queue.
    } catch (const std::exception&) {}
}
