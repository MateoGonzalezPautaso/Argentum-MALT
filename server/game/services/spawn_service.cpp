#include "spawn_service.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../game_formulas.h"

SpawnService::SpawnService(std::map<uint16_t, EnemyNpc>& enemy_npcs,
                           std::unordered_map<std::string, Map>& maps, uint16_t& next_npc_id,
                           Rng& rng, const std::map<uint16_t, Player>& players,
                           const BalanceConfig& balance, const ItemCatalog& item_catalog,
                           const MobSpawnConfig& mob_spawn,
                           const std::vector<NpcTemplate>& world_npc_templates,
                           const std::vector<NpcTemplate>& dungeon_npc_templates):
        enemy_npcs_(enemy_npcs),
        maps_(maps),
        next_npc_id_(next_npc_id),
        rng_(rng),
        players_(players),
        balance_(balance),
        item_catalog_(item_catalog),
        mob_spawn_(mob_spawn),
        world_npc_templates_(world_npc_templates),
        dungeon_npc_templates_(dungeon_npc_templates) {}

EntitySpawnEvent SpawnService::make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const {
    return EntitySpawnEvent{
            .entity_id = npc_id,
            .entity_type = EntityType::NPC,
            .entity_pos = npc.get_pos(),
            .entity_dir = Direction::SOUTH,
            .entity_name = npc.get_name(),
            .entity_race = Race::HUMAN,
            .entity_class = PlayerClass::WARRIOR,
            .sprite_id = npc.get_sprite_id(),
            .clan_name = "",
    };
}

std::vector<uint16_t> SpawnService::get_player_ids_on_map(const std::string& map_name) const {
    std::vector<uint16_t> ids;
    for (const auto& [id, player]: players_) {
        if (player.get_current_map() == map_name)
            ids.push_back(id);
    }
    return ids;
}

EnemyNpc SpawnService::create_random_npc(Position pos, uint8_t level, bool dungeon) {
    const std::vector<NpcTemplate>& pool =
            dungeon ? dungeon_npc_templates_ : world_npc_templates_;
    if (pool.empty())
        return EnemyNpc(pos, GameFormulas::npc_hp(balance_.npc_fallback_base_hp, level),
                        GameFormulas::npc_damage(balance_.npc_fallback_base_damage, level), rng_,
                        item_catalog_, level, "NPC");
    int roll = rng_.get_random_int(0, static_cast<int>(pool.size()) - 1);
    const NpcTemplate& t = pool[roll];
    return EnemyNpc(pos, GameFormulas::npc_hp(t.base_hp, level),
                    GameFormulas::npc_damage(t.base_damage, level), rng_, item_catalog_, level,
                    t.name, t.sprite_id, t.speed);
}

CommandResult SpawnService::spawn_mobs() {
    ++mob_spawn_tick_;
    CommandResult result;

    for (auto& [map_name, map]: maps_) {
        const TilemapConfig& cfg = map.config();
        if (cfg.mob_spawn_limit <= 0 || cfg.mob_spawn_interval_ticks <= 0)
            continue;

        if (mob_spawn_tick_ % cfg.mob_spawn_interval_ticks != 0)
            continue;

        int npc_count = 0;
        for (const auto& [id, npc]: enemy_npcs_) {
            if (!npc.is_dead() && npc.get_current_map() == map_name)
                ++npc_count;
        }

        if (npc_count >= cfg.mob_spawn_limit)
            continue;

        std::optional<std::pair<int, int>> pos_opt;
        for (const auto& [pid, player]: players_) {
            if (player.get_current_map() != map_name || player.is_dead())
                continue;
            pos_opt = map.find_random_mob_spawn_pos_near(rng_, player.pos_x(), player.pos_y(),
                                                         mob_spawn_.spawn_radius);
            if (pos_opt.has_value())
                break;
        }
        if (!pos_opt.has_value())
            pos_opt = map.find_random_mob_spawn_pos(rng_);
        if (!pos_opt.has_value())
            continue;

        Position spawn_pos{static_cast<uint16_t>(pos_opt->first),
                           static_cast<uint16_t>(pos_opt->second)};

        bool is_dungeon = cfg.map_type == MapType::DUNGEON;
        const MobLevelRange& level_range = is_dungeon ? mob_spawn_.dungeon : mob_spawn_.world;
        uint8_t level = static_cast<uint8_t>(
                rng_.get_random_int(level_range.min_level, level_range.max_level));
        EnemyNpc npc = create_random_npc(spawn_pos, level, is_dungeon);
        uint16_t npc_id = next_npc_id_++;
        npc.set_current_map(map_name);

        EntitySpawnEvent spawn_ev = make_npc_spawn(npc, npc_id);
        enemy_npcs_.emplace(npc_id, std::move(npc));

        for (uint16_t pid: get_player_ids_on_map(map_name))
            result.targeted_events[pid].push_back(spawn_ev);
    }

    return result;
}
