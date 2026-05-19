#include "client_handler.h"

ClientHandler::ClientHandler(uint16_t player_id, Socket&& skt, Queue<PlayerCommand>& input_queue):
        player_id(player_id),
        protocol(std::move(skt)),
        sender(protocol, output_queue),
        receiver(player_id, protocol, input_queue) {}

void ClientHandler::start() {
    sender.start();
    receiver.start();
}

void ClientHandler::push_event(const ServerEvent& ev) { output_queue.push(ev); }

void ClientHandler::stop() {
    // Close output_queue first, unblocks Sender if it's waiting on queue.pop().
    output_queue.close();
    // Shutdown the socket, unblocks Receiver if it's waiting on recv_command().
    protocol.shutdown();
}

void ClientHandler::join() {
    sender.join();
    receiver.join();
}

// Both threads must be alive for the handler to be considered connected.
// If either exits (send failure or client disconnect), the handler is dead.
bool ClientHandler::is_alive() const {
    return sender.is_alive() && receiver.is_alive();
}
