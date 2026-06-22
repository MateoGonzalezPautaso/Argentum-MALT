#include "map_transition_service.h"

#include <cmath>
#include <string>
#include <vector>

#include "../entity_event_factory.h"
#include "../player_registry.h"

MapTransitionService::MapTransitionService(std::map<uint16_t, Player>& players,
                                           std::unordered_map<std::string, Map>& maps,
                                           const std::map<uint16_t, EnemyNpc>& enemy_npcs,
                                           PlayerDataService& player_data_service,
                                           const BalanceConfig& balance, int sprite_width,
                                           int sprite_height,
                                           GroundItemService& ground_item_service):
        players_(players),
        maps_(maps),
        enemy_npcs_(enemy_npcs),
        player_data_service_(player_data_service),
        balance_(balance),
        sprite_width_(sprite_width),
        sprite_height_(sprite_height),
        ground_item_service_(ground_item_service) {}

std::vector<uint16_t> MapTransitionService::get_player_ids_on_map(
        const std::string& map_name) const {
    return PlayerRegistry(players_).ids_on_map(map_name);
}

void MapTransitionService::append_existing_entities(std::vector<ServerEvent>& events,
                                                    uint16_t exclude_id,
                                                    const std::string& map_name) const {
    for (const auto& [id, player]: players_) {
        if (id == exclude_id)
            continue;
        if (player.get_current_map() != map_name)
            continue;
        events.push_back(EntityEventFactory::make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs_) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != map_name)
            continue;
        events.push_back(EntityEventFactory::make_npc_spawn(npc, id));
    }
    std::vector<ServerEvent> items = ground_item_service_.make_existing_ground_items(map_name);
    events.insert(events.end(), items.begin(), items.end());
}

bool MapTransitionService::try_map_transition(Player& player, CommandResult& result) {
    const std::string old_map_name = player.get_current_map();
    auto map_it = maps_.find(old_map_name);
    if (map_it == maps_.end())
        return false;

    const int foot_x = static_cast<int>(player.pos_x()) + sprite_width_ / 2;
    const int foot_y = static_cast<int>(player.pos_y()) + sprite_height_;

    const PropGrid::Entry* entry = map_it->second.prop_grid().find_transition_at(foot_x, foot_y);
    if (!entry)
        return false;

    do_transition(player, result, *entry, old_map_name);
    return true;
}

void MapTransitionService::do_transition(Player& player, CommandResult& result,
                                         const PropGrid::Entry& entry,
                                         const std::string& old_map_name) {
    auto dest_it = maps_.find(entry.transition_map());
    if (dest_it == maps_.end())
        return;

    Position spawn = compute_spawn_position(dest_it->second, old_map_name, entry);

    despawn_player(result, player.get_id(), old_map_name);

    player.set_current_map(entry.transition_map());
    player.set_pos(spawn.x, spawn.y);

    player_data_service_.save_player(player);

    notify_player_transition(result, player, entry.transition_map(), spawn);
    notify_others_spawn(result, player, entry.transition_map());
}

Position MapTransitionService::compute_spawn_position(const Map& dest_map,
                                                      const std::string& old_map_name,
                                                      const PropGrid::Entry& source_entry) const {
    int cx, cy, hb_left, hb_bottom;
    if (dest_map.prop_grid().find_first_transition(old_map_name, cx, cy, hb_left, hb_bottom)) {
        int spawn_x = hb_left - dest_map.tile_size();
        int spawn_y = hb_bottom - dest_map.tile_size();
        return {static_cast<uint16_t>(spawn_x), static_cast<uint16_t>(spawn_y)};
    }
    int x = source_entry.transition_x();
    int y = source_entry.transition_y();
    if (x == 0 && y == 0) {
        x = dest_map.tile_size() * 2;
        y = dest_map.tile_size() * 2;
    }
    return {static_cast<uint16_t>(x), static_cast<uint16_t>(y)};
}

void MapTransitionService::despawn_player(CommandResult& result, uint16_t player_id,
                                          const std::string& old_map_name) const {
    EntityDespawnEvent despawn{.entity_id = player_id};
    for (uint16_t pid: get_player_ids_on_map(old_map_name)) {
        if (pid == player_id)
            continue;
        result.targeted_events[pid].push_back(despawn);
    }
}

void MapTransitionService::notify_player_transition(CommandResult& result, const Player& player,
                                                    const std::string& map_name,
                                                    Position spawn) const {
    result.private_events.push_back(MapTransitionEvent{
            .map_name = map_name,
            .pos_x = spawn.x,
            .pos_y = spawn.y,
    });
    append_existing_entities(result.private_events, player.get_id(), map_name);
}

void MapTransitionService::notify_others_spawn(CommandResult& result, const Player& player,
                                               const std::string& map_name) const {
    EntitySpawnEvent spawn = EntityEventFactory::make_entity_spawn(player);
    for (uint16_t pid: get_player_ids_on_map(map_name)) {
        if (pid == player.get_id())
            continue;
        result.targeted_events[pid].push_back(spawn);
    }
}

CommandResult MapTransitionService::handle_change_map(uint16_t player_id, const ChangeMapCmd& cmd) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};

    Player& player = it->second;
    const std::string& old_map_name = player.get_current_map();
    auto map_it = maps_.find(old_map_name);
    if (map_it == maps_.end())
        return {};

    const TilemapConfig& cfg = map_it->second.config();
    const int range = cfg.tile_size * balance_.npc_interaction_range_tiles;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    const PropGrid::Entry* entry =
            map_it->second.prop_grid().find_closest(cmd.prop_name, px, py, range);
    if (!entry || entry->transition_map().empty())
        return {};

    CommandResult result;
    do_transition(player, result, *entry, old_map_name);
    return result;
}
