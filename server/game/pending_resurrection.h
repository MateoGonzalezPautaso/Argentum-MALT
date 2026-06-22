#ifndef PENDING_RESURRECTION_H
#define PENDING_RESURRECTION_H

#include <cstdint>
#include <string>

#include "../../common/messages.h"

struct PendingResurrection {
    uint32_t remaining_ticks;
    std::string target_map;
    Position target_pos;
};

#endif  // PENDING_RESURRECTION_H
