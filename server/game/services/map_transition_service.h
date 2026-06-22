#ifndef MAP_TRANSITION_SERVICE_H
#define MAP_TRANSITION_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../enemy_npc.h"
#include "../map.h"
#include "../player.h"
#include "../player_data_service.h"
#include "../prop_grid.h"

#include "ground_item_service.h"

class MapTransitionService {
public:
    MapTransitionService(std::map<uint16_t, Player>& players,
                         std::unordered_map<std::string, Map>& maps,
                         const std::map<uint16_t, EnemyNpc>& enemy_npcs,
                         PlayerDataService& player_data_service, const BalanceConfig& balance,
                         int sprite_width, int sprite_height,
                         GroundItemService& ground_item_service);

    // Intenta detectar y ejecutar una transición de mapa para el jugador.
    // Devuelve true si hubo transición y rellena result.
    bool try_map_transition(Player& player, CommandResult& result);

    CommandResult handle_change_map(uint16_t player_id, const ChangeMapCmd& cmd);

private:
    void do_transition(Player& player, CommandResult& result, const PropGrid::Entry& entry,
                       const std::string& old_map_name);

    Position compute_spawn_position(const Map& dest_map, const std::string& old_map_name,
                                    const PropGrid::Entry& source_entry) const;

    void despawn_player(CommandResult& result, uint16_t player_id,
                        const std::string& old_map_name) const;

    void notify_player_transition(CommandResult& result, const Player& player,
                                  const std::string& map_name, Position spawn) const;

    void notify_others_spawn(CommandResult& result, const Player& player,
                             const std::string& map_name) const;

    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;
    void append_existing_entities(std::vector<ServerEvent>& events, uint16_t exclude_id,
                                  const std::string& map_name) const;
    EntitySpawnEvent make_entity_spawn(const Player& p) const;
    EntitySpawnEvent make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const;

    std::map<uint16_t, Player>& players_;
    std::unordered_map<std::string, Map>& maps_;
    const std::map<uint16_t, EnemyNpc>& enemy_npcs_;
    PlayerDataService& player_data_service_;
    const BalanceConfig& balance_;
    int sprite_width_;
    int sprite_height_;
    GroundItemService& ground_item_service_;
};

#endif  // MAP_TRANSITION_SERVICE_H
