#ifndef COMMAND_RESULT_H
#define COMMAND_RESULT_H

#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../common/messages.h"

struct CommandResult {
    std::vector<ServerEvent> private_events{};    // solo al que emitió el comando
    std::vector<ServerEvent> broadcast_events{};  // a todos los clientes (chat, sistema)
    std::map<uint16_t, std::vector<ServerEvent>> targeted_events{};  // por ID de jugador
    std::vector<ServerEvent> map_events{};  // a los jugadores en el mismo mapa
    // items que deben quedar en el suelo, agrupados por mapa
    std::map<std::string, std::vector<ItemDroppedEvent>> ground_drops{};

    // Absorbe todos los eventos de `other` en este resultado.
    // Solo mergea targeted_events y broadcast_events (los más usados en tick()).
    static CommandResult with_msg(const std::string& text) {
        return {.private_events = {ChatMsgEvent{ChatMsgType::SYSTEM, "", text}}};
    }

    CommandResult& merge(CommandResult&& other) {
        for (auto& [pid, evs]: other.targeted_events) {
            auto& target = targeted_events[pid];
            target.insert(target.end(), std::make_move_iterator(evs.begin()),
                          std::make_move_iterator(evs.end()));
        }
        broadcast_events.insert(broadcast_events.end(),
                                std::make_move_iterator(other.broadcast_events.begin()),
                                std::make_move_iterator(other.broadcast_events.end()));
        return *this;
    }
};

#endif
