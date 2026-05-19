#include "acceptor.h"

#include <sys/socket.h>

Acceptor::Acceptor(Socket& listener, Queue<PlayerCommand>& input_queue,
                   ClientListMonitor& monitor):
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
    listener.shutdown(SHUT_RDWR);  // unblocks the blocking accept() call
}
