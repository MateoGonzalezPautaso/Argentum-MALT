#ifndef PLAYER_COMMAND_H
#define PLAYER_COMMAND_H

#include <cstdint>

#include "../../common/messages.h"

// A ClientCommand tagged with the player_id that sent it
struct PlayerCommand {
    uint16_t player_id;
    ClientCommand cmd;
};

#endif  // PLAYER_COMMAND_H
