#include "client_list_monitor.h"

uint16_t ClientListMonitor::add(Socket&& skt, Queue<PlayerCommand>& input_queue) {
    std::lock_guard<std::mutex> lock(mtx);

    uint16_t id = next_id++;
    auto handler = std::make_unique<ClientHandler>(id, std::move(skt), input_queue);
    handler->start();
    clients.emplace(id, std::move(handler));
    return id;
}

void ClientListMonitor::broadcast(const ServerEvent& event) {
    std::lock_guard<std::mutex> lock(mtx);

    for (auto& [id, client] : clients) {
        try {
            client->push_event(event);
        } catch (const ClosedQueue&) {
            // Client is shutting down, skip instead of aborting the broadcast.
        }
    }
}

void ClientListMonitor::clean_dead() {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = clients.begin();
    while (it != clients.end()) {
        ClientHandler& client = *it->second;
        // *it->second dereferences the unique_ptr, giving the ClientHandler

        if (!client.is_alive()) {
            client.stop();
            client.join();
            it = clients.erase(it);
        } else {
            it++;
        }
    }
}

void ClientListMonitor::stop_all() {
    std::lock_guard<std::mutex> lock(mtx);

    for (auto& [id, client] : clients) {
        client->stop();
        client->join();
    }

    clients.clear();
}
