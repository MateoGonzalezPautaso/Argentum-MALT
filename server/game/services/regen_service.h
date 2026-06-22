#ifndef REGEN_SERVICE_H
#define REGEN_SERVICE_H

#include <cstdint>
#include <functional>
#include <map>
#include <unordered_map>

#include "../../core/config.h"
#include "../command_result.h"
#include "../player.h"

class RegenService {
public:
    RegenService(std::map<uint16_t, Player>& players,
                 const BalanceConfig& balance,
                 int tick_rate_hz,
                 std::unordered_map<uint16_t, double>& hp_regen_accum,
                 std::unordered_map<uint16_t, double>& mana_regen_accum);

    CommandResult apply_regen();

private:
    bool apply_regen_channel(double& accum, double rate,
                             uint32_t cur, uint32_t max,
                             std::function<void(uint32_t)> apply_fn);

    std::map<uint16_t, Player>& players_;
    const BalanceConfig& balance_;
    int tick_rate_hz_;
    std::unordered_map<uint16_t, double>& hp_regen_accum_;
    std::unordered_map<uint16_t, double>& mana_regen_accum_;
};

#endif  // REGEN_SERVICE_H
