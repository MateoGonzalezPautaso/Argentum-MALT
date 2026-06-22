#ifndef COMMAND_RESULT_H
#define COMMAND_RESULT_H

#include <map>
#include <string>
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
    CommandResult& merge(CommandResult&& other) {
        for (auto& [pid, evs]: other.targeted_events)
            for (auto& ev: evs) targeted_events[pid].push_back(std::move(ev));
        for (auto& ev: other.broadcast_events) broadcast_events.push_back(std::move(ev));
        return *this;
    }
};

#endif
