#include "regen_service.h"

#include "../game_formulas.h"

RegenService::RegenService(std::map<uint16_t, Player>& players,
                           const BalanceConfig& balance,
                           int tick_rate_hz,
                           std::unordered_map<uint16_t, double>& hp_regen_accum,
                           std::unordered_map<uint16_t, double>& mana_regen_accum):
        players_(players),
        balance_(balance),
        tick_rate_hz_(tick_rate_hz),
        hp_regen_accum_(hp_regen_accum),
        mana_regen_accum_(mana_regen_accum) {}

bool RegenService::apply_regen_channel(double& accum, double rate,
                                       uint32_t cur, uint32_t max,
                                       std::function<void(uint32_t)> apply_fn) {
    // rate is in units/second; dt converts to per-tick gain.
    const double dt = 1.0 / tick_rate_hz_;
    accum += rate * dt;
    if (accum >= 1.0 && cur < max) {
        uint32_t gain = static_cast<uint32_t>(accum);
        apply_fn(gain);
        accum -= gain;
        return true;
    } else if (accum >= 1.0) {
        accum = 0.0;
    }
    return false;
}

CommandResult RegenService::apply_regen() {
    CommandResult result;

    for (auto& [id, player]: players_) {
        if (player.is_dead())
            continue;

        bool changed = false;

        double hp_rate = GameFormulas::hp_regen_per_second(balance_, player.get_race());
        changed |= apply_regen_channel(
                hp_regen_accum_[id],
                hp_rate,
                player.get_hp_current(),
                player.get_hp_max(),
                [&player](uint32_t gain) { player.heal(gain); });

        double mana_rate = GameFormulas::mana_regen_per_second(balance_, player.get_race());
        if (player.get_is_meditating())
            mana_rate += GameFormulas::meditation_mana_per_second(balance_, player.get_race(),
                                                                  player.get_player_class());
        changed |= apply_regen_channel(
                mana_regen_accum_[id],
                mana_rate,
                player.get_mana_current(),
                player.get_mana_max(),
                [&player](uint32_t gain) { player.restore_mana(gain); });

        if (changed) {
            HealReceivedEvent ev{id, player.get_hp_current(), player.get_mana_current()};
            result.broadcast_events.push_back(ev);
        }
    }

    return result;
}
