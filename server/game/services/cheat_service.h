#ifndef CHEAT_SERVICE_H
#define CHEAT_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../player.h"
#include "../player_data_service.h"

class CombatController;

class CheatService {
public:
    CheatService(std::map<uint16_t, Player>& players, const BalanceConfig& balance,
                 const ItemCatalog& item_catalog, PlayerDataService& player_data_service,
                 CombatController& combat_controller, const std::vector<std::string>& help_lines,
                 bool cheats_enabled);

    // Gate centralizado: si cheats_enabled == false devuelve CommandResult{} sin
    // llegar a ninguno de los handlers internos.
    CommandResult dispatch_cheat_infinite_hp(uint16_t player_id);
    CommandResult dispatch_cheat_infinite_mana(uint16_t player_id);
    CommandResult dispatch_cheat_die(uint16_t player_id);
    CommandResult dispatch_cheat_level_up(uint16_t player_id);
    CommandResult dispatch_cheat_level_down(uint16_t player_id);
    CommandResult dispatch_cheat_add_gold(uint16_t player_id);
    CommandResult dispatch_cheat_reset_gold(uint16_t player_id);
    CommandResult dispatch_cheat_velocity(uint16_t player_id);
    CommandResult dispatch_cheat_revive(uint16_t player_id);
    CommandResult dispatch_cheat_fill_inventory(uint16_t player_id);
    CommandResult dispatch_cheat_clear_inventory(uint16_t player_id);
    CommandResult dispatch_cheat_reset_mana(uint16_t player_id);

    CommandResult handle_help();

private:
    std::map<uint16_t, Player>& players_;
    const BalanceConfig& balance_;
    const ItemCatalog& item_catalog_;
    PlayerDataService& player_data_service_;
    CombatController& combat_controller_;
    const std::vector<std::string>& help_lines_;
    bool cheats_enabled_;

    PlayerStatsEvent make_player_stats_event(const Player& p) const;

    CommandResult handle_cheat_infinite_hp(uint16_t player_id);
    CommandResult handle_cheat_infinite_mana(uint16_t player_id);
    CommandResult handle_cheat_die(uint16_t player_id);
    CommandResult handle_cheat_level_up(uint16_t player_id);
    CommandResult handle_cheat_level_down(uint16_t player_id);
    CommandResult handle_cheat_add_gold(uint16_t player_id);
    CommandResult handle_cheat_reset_gold(uint16_t player_id);
    CommandResult handle_cheat_velocity(uint16_t player_id);
    CommandResult handle_cheat_revive(uint16_t player_id);
    CommandResult handle_cheat_fill_inventory(uint16_t player_id);
    CommandResult handle_cheat_clear_inventory(uint16_t player_id);
    CommandResult handle_cheat_reset_mana(uint16_t player_id);
};

#endif  // CHEAT_SERVICE_H
