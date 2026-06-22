#ifndef SPAWN_SERVICE_H
#define SPAWN_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../common/messages.h"
#include "../../../common/rng.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../enemy_npc.h"
#include "../map.h"
#include "../player.h"

class SpawnService {
public:
    SpawnService(std::map<uint16_t, EnemyNpc>& enemy_npcs,
                 std::unordered_map<std::string, Map>& maps, uint16_t& next_npc_id, Rng& rng,
                 const std::map<uint16_t, Player>& players, const BalanceConfig& balance,
                 const ItemCatalog& item_catalog, const MobSpawnConfig& mob_spawn,
                 const std::vector<NpcTemplate>& world_npc_templates,
                 const std::vector<NpcTemplate>& dungeon_npc_templates);

    // Llamado cada tick; devuelve eventos de spawn a enviar a jugadores en el mapa.
    CommandResult spawn_mobs();

private:
    EnemyNpc create_random_npc(Position pos, uint8_t level, bool dungeon);
    EntitySpawnEvent make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const;
    std::vector<uint16_t> get_player_ids_on_map(const std::string& map_name) const;

    std::map<uint16_t, EnemyNpc>& enemy_npcs_;
    std::unordered_map<std::string, Map>& maps_;
    uint16_t& next_npc_id_;
    Rng& rng_;
    const std::map<uint16_t, Player>& players_;
    const BalanceConfig& balance_;
    const ItemCatalog& item_catalog_;
    const MobSpawnConfig& mob_spawn_;
    const std::vector<NpcTemplate>& world_npc_templates_;
    const std::vector<NpcTemplate>& dungeon_npc_templates_;
    uint32_t mob_spawn_tick_ = 0;
};

#endif  // SPAWN_SERVICE_H
