#ifndef COMMAND_RESULT_H
#define COMMAND_RESULT_H

#include <map>
#include <vector>

#include "../../common/messages.h"

struct CommandResult {
    std::vector<ServerEvent> private_events;   // solo al que emitió el comando
    std::vector<ServerEvent> broadcast_events; // a todos los clientes
    std::map<uint16_t, std::vector<ServerEvent>> targeted_events; // por ID de jugador
};

#endif
