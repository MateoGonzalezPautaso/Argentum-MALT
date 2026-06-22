#include "ground_item_service.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include "../player_registry.h"

GroundItemService::GroundItemService(std::map<uint16_t, Player>& players,
                                     std::unordered_map<std::string, Map>& maps,
                                     const ItemCatalog& item_catalog):
        players_(players), maps_(maps), item_catalog_(item_catalog) {}

std::pair<int, int> GroundItemService::tile_cell(const Map& map, int px, int py) {
    const int tile_size = map.tile_size();
    return {px / tile_size, py / tile_size};
}

std::vector<uint16_t> GroundItemService::get_player_ids_on_map(const std::string& map_name) const {
    return PlayerRegistry(players_).ids_on_map(map_name);
}

std::vector<ServerEvent> GroundItemService::make_existing_ground_items(
        const std::string& map_name) const {
    std::vector<ServerEvent> events;
    auto map_it = ground_items_.find(map_name);
    if (map_it == ground_items_.end())
        return events;

    for (const auto& [cell, vec]: map_it->second) {
        events.resize(vec.size());
        std::copy(vec.begin(), vec.end(), events.begin());
    }
    return events;
}

void GroundItemService::commit_ground_drops(
        CommandResult& result, const std::map<std::string, std::vector<ItemDroppedEvent>>& drops) {
    for (const auto& [map_name, items]: drops) {
        auto map_it = maps_.find(map_name);
        if (map_it == maps_.end())
            continue;

        std::vector<uint16_t> player_ids = get_player_ids_on_map(map_name);
        for (const auto& item: items) {
            auto cell = tile_cell(map_it->second, item.pos.x, item.pos.y);
            ground_items_[map_name][cell].push_back(item);

            for (uint16_t pid: player_ids) result.targeted_events[pid].push_back(item);
        }
    }
}

CommandResult GroundItemService::handle_pickup_item(uint16_t player_id, const PickupItemCmd& cmd) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;

    if (player.is_dead()) {
        return CommandResult::with_msg("Los fantasmas no pueden recoger objetos");
    }

    const std::string& map_name = player.get_current_map();
    auto map_it = maps_.find(map_name);
    if (map_it == maps_.end())
        return {};
    const Map& map = map_it->second;
    auto cell = tile_cell(map, static_cast<int>(player.pos_x()), static_cast<int>(player.pos_y()));

    auto ground_map_it = ground_items_.find(map_name);
    if (ground_map_it == ground_items_.end())
        return CommandResult::with_msg("No hay nada para recoger aquí");
    auto cell_it = ground_map_it->second.find(cell);
    if (cell_it == ground_map_it->second.end() || cell_it->second.empty())
        return CommandResult::with_msg("No hay nada para recoger aquí");

    std::vector<ItemDroppedEvent>& vec = cell_it->second;
    std::vector<ItemDroppedEvent>::iterator pick_it;
    if (cmd.item_name.empty()) {
        pick_it = vec.begin();
    } else {
        pick_it = std::find_if(vec.begin(), vec.end(), [&](const ItemDroppedEvent& g) {
            if (g.item_name.size() != cmd.item_name.size())
                return false;
            return std::equal(g.item_name.begin(), g.item_name.end(), cmd.item_name.begin(),
                              [](unsigned char a, unsigned char b) {
                                  return std::tolower(a) == std::tolower(b);
                              });
        });
        if (pick_it == vec.end()) {
            return CommandResult::with_msg("No hay '" + cmd.item_name + "' en el piso aquí");
        }
    }

    const ItemDroppedEvent ground_item = *pick_it;
    const bool is_gold = ground_item.item_type == ItemType::GOLD_DROP;
    if (!is_gold && !player.add_item(ground_item.item_type, ground_item.item_name)) {
        return CommandResult::with_msg("Inventario lleno");
    }
    vec.erase(pick_it);
    if (vec.empty())
        ground_map_it->second.erase(cell_it);

    CommandResult result;
    if (is_gold) {
        player.gain_gold(ground_item.amount);
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "Recogiste " + std::to_string(ground_item.amount) + " de oro"};
        result.private_events = {msg, GoldUpdateEvent{player.get_gold()}};
    } else {
        InventoryUpdateEvent inv_ev{player.dump_inventory()};
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Recogiste " + ground_item.item_name};
        result.private_events = {msg, inv_ev};
    }
    result.map_events = {
            ItemPickedEvent{ground_item.pos, ground_item.item_name, ground_item.amount}};
    return result;
}

CommandResult GroundItemService::handle_drop_item(uint16_t player_id, const DropItemCmd& cmd) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;

    if (player.is_dead()) {
        return CommandResult::with_msg("Los fantasmas no pueden tirar objetos");
    }

    const Item* item_def = item_catalog_.find_by_name(cmd.item_name);
    if (!item_def) {
        return CommandResult::with_msg("Objeto '" + cmd.item_name + "' no encontrado");
    }

    auto slot_opt = player.find_slot_by_type(item_def->type);
    if (!slot_opt) {
        return CommandResult::with_msg("No tenés '" + item_def->name + "' en el inventario");
    }

    const std::string& map_name = player.get_current_map();
    auto map_it = maps_.find(map_name);
    if (map_it == maps_.end())
        return {};
    const Map& map = map_it->second;
    auto cell = tile_cell(map, static_cast<int>(player.pos_x()), static_cast<int>(player.pos_y()));

    Position pos{player.pos_x(), player.pos_y()};

    player.remove_inventory_item(static_cast<uint8_t>(slot_opt->slot_index));
    ItemDroppedEvent drop_ev{pos, item_def->type, item_def->name};
    ground_items_[map_name][cell].push_back(drop_ev);

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Tiraste " + item_def->name};
    CommandResult result;
    result.private_events = {msg, inv_ev};
    result.map_events = {drop_ev};
    return result;
}
