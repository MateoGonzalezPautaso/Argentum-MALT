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
};

#endif
