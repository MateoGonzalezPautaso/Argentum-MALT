#include "resurrection_service.h"

#include <cmath>
#include <format>

#include "../entity_event_factory.h"
#include "../player_registry.h"
#include "../prop_names.h"

ResurrectionService::ResurrectionService(
        std::map<uint16_t, Player>& players,
        std::unordered_map<std::string, Map>& maps,
        std::map<uint16_t, EnemyNpc>& enemy_npcs,
        std::unordered_map<uint16_t, PendingResurrection>& pending_resurrections,
        GroundItemService& ground_item_service,
        const BalanceConfig& balance,
        const MessagesConfig& msgs,
        int sprite_width,
        int sprite_height,
        int tick_rate_hz):
        players_(players),
        maps_(maps),
        enemy_npcs_(enemy_npcs),
        pending_resurrections_(pending_resurrections),
        ground_item_service_(ground_item_service),
        balance_(balance),
        msgs_(msgs),
        sprite_width_(sprite_width),
        sprite_height_(sprite_height),
        tick_rate_hz_(tick_rate_hz) {}

std::vector<uint16_t> ResurrectionService::get_player_ids_on_map(
        const std::string& map_name) const {
    return PlayerRegistry(players_).ids_on_map(map_name);
}

std::vector<ServerEvent> ResurrectionService::make_existing_spawns(
        uint16_t exclude_id, const std::string& map_name) const {
    std::vector<ServerEvent> spawns;
    for (const auto& [id, player]: players_) {
        if (id == exclude_id)
            continue;
        if (player.get_current_map() != map_name)
            continue;
        spawns.push_back(EntityEventFactory::make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs_) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != map_name)
            continue;
        spawns.push_back(EntityEventFactory::make_npc_spawn(npc, id));
    }
    return spawns;
}

void ResurrectionService::append_existing_entities(std::vector<ServerEvent>& events,
                                                   uint16_t exclude_id,
                                                   const std::string& map_name) const {
    std::vector<ServerEvent> spawns = make_existing_spawns(exclude_id, map_name);
    events.insert(events.end(), spawns.begin(), spawns.end());

    std::vector<ServerEvent> items = ground_item_service_.make_existing_ground_items(map_name);
    events.insert(events.end(), items.begin(), items.end());
}

CommandResult ResurrectionService::handle_resurrect(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};

    Player& player = it->second;
    if (!player.is_dead()) {
        return CommandResult::with_msg(msgs_.not_dead);
    }

    if (pending_resurrections_.contains(player_id)) {
        return CommandResult::with_msg(msgs_.already_resurrecting);
    }

    const std::string& current_map = player.get_current_map();
    auto map_it = maps_.find(current_map);
    if (map_it == maps_.end())
        return {};

    const int tile_size = map_it->second.tile_size();
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());
    const int range = tile_size * balance_.npc_interaction_range_tiles;

    // If the ghost is near a sacerdote, resurrect immediately
    if (map_it->second.prop_grid().is_in_range_of(std::string(PropNames::PRIEST), px, py, range)) {
        player.resurrect();
        PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.priest_resurrect};
        CommandResult result;
        result.private_events = {msg};
        result.map_events = {respawn};
        return result;
    }

    std::string target_map;
    uint32_t wait_ticks;
    int san_cx, san_cy;

    const PropGrid::Entry* san_entry =
            map_it->second.prop_grid().find_closest(std::string(PropNames::HEALER), px, py);
    if (san_entry) {
        target_map = current_map;
        san_cx = san_entry->center_x;
        san_cy = san_entry->center_y;
        int dx = px - san_cx;
        int dy = py - san_cy;
        int dist_px = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
        int dist_tiles = dist_px / tile_size;
        wait_ticks = static_cast<uint32_t>(dist_tiles * tick_rate_hz_);
    } else {
        auto main_it = maps_.find(balance_.starting_map);
        if (main_it == maps_.end())
            return {};
        san_entry =
                main_it->second.prop_grid().find_closest(std::string(PropNames::HEALER), 0, 0);
        if (!san_entry)
            return {};
        target_map = balance_.starting_map;
        san_cx = san_entry->center_x;
        san_cy = san_entry->center_y;
        wait_ticks =
                static_cast<uint32_t>(balance_.default_resurrect_wait_seconds * tick_rate_hz_);
    }

    Position target_pos{static_cast<uint16_t>(san_cx - sprite_width_ / 2),
                        static_cast<uint16_t>(san_cy - sprite_height_)};

    pending_resurrections_[player_id] = {wait_ticks, target_map, target_pos};

    uint32_t remaining_tiles = wait_ticks / tick_rate_hz_;
    return CommandResult::with_msg(
            std::vformat(msgs_.resurrect_countdown, std::make_format_args(remaining_tiles)));
}

CommandResult ResurrectionService::process_pending_resurrections() {
    CommandResult result;

    for (auto it = pending_resurrections_.begin(); it != pending_resurrections_.end();) {
        auto& [player_id, pending] = *it;

        if (pending.remaining_ticks > 0) {
            --pending.remaining_ticks;
            ++it;
            continue;
        }

        auto player_it = players_.find(player_id);
        if (player_it == players_.end()) {
            it = pending_resurrections_.erase(it);
            continue;
        }

        Player& player = player_it->second;
        const std::string old_map = player.get_current_map();
        const bool needs_map_transition = (pending.target_map != old_map);

        if (needs_map_transition) {
            EntityDespawnEvent despawn{player_id};
            for (uint16_t pid: get_player_ids_on_map(old_map)) {
                if (pid != player_id)
                    result.targeted_events[pid].push_back(despawn);
            }

            player.set_current_map(pending.target_map);
            player.set_pos(pending.target_pos.x, pending.target_pos.y);

            player.resurrect();

            std::vector<ServerEvent> private_events;
            private_events.push_back(MapTransitionEvent{pending.target_map, pending.target_pos.x,
                                                        pending.target_pos.y});
            append_existing_entities(private_events, player_id, pending.target_map);
            result.targeted_events[player_id] = std::move(private_events);

            EntitySpawnEvent spawn = EntityEventFactory::make_entity_spawn(player);
            PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
            for (uint16_t pid: get_player_ids_on_map(pending.target_map)) {
                if (pid != player_id)
                    result.targeted_events[pid].push_back(spawn);
                result.targeted_events[pid].push_back(respawn);
            }
        } else {
            player.set_pos(pending.target_pos.x, pending.target_pos.y);
            player.resurrect();

            EntitySpawnEvent spawn = EntityEventFactory::make_entity_spawn(player);
            EntityMoveEvent move_ev{player_id, player.get_pos(), player.get_dir()};
            PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
            for (uint16_t pid: get_player_ids_on_map(pending.target_map)) {
                if (pid != player_id)
                    result.targeted_events[pid].push_back(spawn);
                result.targeted_events[pid].push_back(move_ev);
                result.targeted_events[pid].push_back(respawn);
            }
        }

        it = pending_resurrections_.erase(it);
    }

    return result;
}
