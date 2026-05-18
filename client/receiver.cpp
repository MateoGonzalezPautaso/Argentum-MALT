#include "receiver.h"

Receiver::Receiver(ClientProtocol& protocol, Queue<ServerEvent>& queue):
        protocol(protocol), queue(queue) {}

void Receiver::run() {
    try {
        while (should_keep_running()) {
            ServerEvent ev = protocol.recv_event();
            queue.push(ev);
        }
    } catch (const ClosedQueue&) {
    } catch (const std::exception&) {}
}
