#ifndef SPELL_SERVICE_H
#define SPELL_SERVICE_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include "../../../common/item_catalog.h"
#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../enemy_npc.h"
#include "../map.h"
#include "../player.h"

class CombatController;
class GroundItemService;

class SpellService {
public:
    SpellService(std::map<uint16_t, Player>& players,
                 const std::map<uint16_t, EnemyNpc>& enemy_npcs,
                 const std::unordered_map<std::string, Map>& maps,
                 const ItemCatalog& item_catalog, CombatController& combat_controller,
                 GroundItemService& ground_item_service, const BalanceConfig& balance,
                 const MessagesConfig& msgs);

    CommandResult handle_cast_spell(uint16_t player_id, const CastSpellCmd& cmd,
                                    uint32_t tick_count);

private:
    std::optional<CommandResult> validate_cast(const Player& player,
                                               const CastSpellCmd& cmd) const;
    const Map& player_map(const Player& p) const;

    std::map<uint16_t, Player>& players_;
    const std::map<uint16_t, EnemyNpc>& enemy_npcs_;
    const std::unordered_map<std::string, Map>& maps_;
    const ItemCatalog& item_catalog_;
    CombatController& combat_controller_;
    GroundItemService& ground_item_service_;
    const BalanceConfig& balance_;
    const MessagesConfig& msgs_;
};

#endif  // SPELL_SERVICE_H
