#ifndef COMMAND_RESULT_H
#define COMMAND_RESULT_H

#include <vector>

#include "../../common/messages.h"

struct CommandResult {
    std::vector<ServerEvent> private_events;
    std::vector<ServerEvent> broadcast_events;
};

#endif
