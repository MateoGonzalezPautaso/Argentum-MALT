#include "acceptor.h"

#include <utility>

#include <sys/socket.h>

Acceptor::Acceptor(Socket& listener, Queue<PlayerCommand>& input_queue, ClientListMonitor& monitor):
        listener(listener), input_queue(input_queue), monitor(monitor) {}

void Acceptor::run() {
    while (should_keep_running()) {
        try {
            Socket client_skt = listener.accept();
            monitor.add(std::move(client_skt), input_queue);
        } catch (...) {
            // listener.shutdown() was called from stop().
            break;
        }
    }
}

void Acceptor::stop() {
    Thread::stop();
    // Called from the Server thread, not from Acceptor's own thread, so it must
    // not touch stream_status. Unblocks the blocking accept() call.
    listener.shutdown_from_other_thread(SHUT_RDWR);
}
