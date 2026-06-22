#ifndef PLAYER_SESSION_SERVICE_H
#define PLAYER_SESSION_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../common/item_catalog.h"
#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../clan_command_handler.h"
#include "../clan_manager.h"
#include "../combat_controller.h"
#include "../command_result.h"
#include "../enemy_npc.h"
#include "../map.h"
#include "../player.h"
#include "../player_data_service.h"

#include "ground_item_service.h"

class PlayerSessionService {
public:
    PlayerSessionService(std::map<uint16_t, Player>& players,
                         std::unordered_map<std::string, uint16_t>& player_name_index,
                         PlayerDataService& player_data_service,
                         std::unordered_map<std::string, Map>& maps,
                         const std::map<uint16_t, EnemyNpc>& enemy_npcs,
                         const BalanceConfig& balance, const InventoryConfig& inventory_config,
                         const ItemCatalog& item_catalog, ClanManager& clan_manager,
                         ClanCommandHandler& clan_handler, CombatController& combat_controller,
                         GroundItemService& ground_item_service);

    CommandResult handle_login(uint16_t player_id, const LoginCmd& cmd);
    CommandResult handle_create_character(uint16_t player_id, const CreateCharacterCmd& cmd);
    CommandResult remove_player(uint16_t player_id);

private:
    LoginOkEvent make_login_ok(const Player& p) const;
    EntitySpawnEvent make_entity_spawn(const Player& p) const;
    EntitySpawnEvent make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const;
    PlayerStatsEvent make_player_stats_event(const Player& p) const;
    void append_existing_entities(std::vector<ServerEvent>& events, uint16_t exclude_id,
                                  const std::string& map_name) const;
    bool is_username_logged_in(const std::string& username) const;

    std::map<uint16_t, Player>& players_;
    std::unordered_map<std::string, uint16_t>& player_name_index_;
    PlayerDataService& player_data_service_;
    std::unordered_map<std::string, Map>& maps_;
    const std::map<uint16_t, EnemyNpc>& enemy_npcs_;
    const BalanceConfig& balance_;
    const InventoryConfig& inventory_config_;
    const ItemCatalog& item_catalog_;
    ClanManager& clan_manager_;
    ClanCommandHandler& clan_handler_;
    CombatController& combat_controller_;
    GroundItemService& ground_item_service_;
};

#endif  // PLAYER_SESSION_SERVICE_H
