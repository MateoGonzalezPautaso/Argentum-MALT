#ifndef COMMAND_RESULT_H
#define COMMAND_RESULT_H

#include <cstdint>
#include <map>
#include <vector>

#include "../common/messages.h"

struct CommandResult {
    std::vector<ServerEvent> private_events;
    std::vector<ServerEvent> broadcast_events;
    std::map<uint16_t, std::vector<ServerEvent>> targeted_events;
};

#endif
