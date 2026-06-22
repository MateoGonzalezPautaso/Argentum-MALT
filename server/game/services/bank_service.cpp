#include "bank_service.h"

#include <format>
#include <string>
#include <unordered_map>

BankService::BankService(std::map<uint16_t, Player>& players,
                         const std::unordered_map<std::string, Map>& maps,
                         const ItemCatalog& item_catalog, const BalanceConfig& balance,
                         const MessagesConfig& msgs):
        players_(players), maps_(maps), item_catalog_(item_catalog), balance_(balance), msgs_(msgs) {}

BankUpdateEvent BankService::make_bank_update_event(const Player& p) const {
    return BankUpdateEvent{p.dump_bank(), p.get_bank_gold()};
}

CommandResult BankService::handle_bank_deposit(uint16_t player_id, const BankDepositCmd& cmd) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;

    if (player.is_dead()) {
        return CommandResult::with_msg(msgs_.ghost_cant_deposit);
    }

    auto map_it = maps_.find(player.get_current_map());
    if (map_it == maps_.end())
        return {};

    const Map& map = map_it->second;
    const int range = map.tile_size() * balance_.merchant.interaction_range_tiles;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    if (!map.prop_grid().is_in_range_of("banquero", px, py, range)) {
        return CommandResult::with_msg(msgs_.no_banker_nearby);
    }

    if (cmd.is_gold) {
        if (cmd.gold_amount == 0 || player.get_gold() < cmd.gold_amount) {
            return CommandResult::with_msg(msgs_.insufficient_gold);
        }
        player.spend_gold(cmd.gold_amount);
        player.add_bank_gold(cmd.gold_amount);

        GoldUpdateEvent gold_ev{player.get_gold()};
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "Depositaste " + std::to_string(cmd.gold_amount) + " de oro"};
        return {.private_events = {msg, gold_ev, make_bank_update_event(player)}};
    }

    const Item* item_def = item_catalog_.find_by_name(cmd.item_name);
    if (!item_def) {
        return CommandResult::with_msg(
                std::vformat(msgs_.item_not_found, std::make_format_args(cmd.item_name)));
    }

    auto slot_opt = player.find_slot_by_type(item_def->type);
    if (!slot_opt) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         std::vformat(msgs_.item_not_in_inventory,
                                      std::make_format_args(item_def->name))};
        return {.private_events = {msg}};
    }

    if (!player.add_to_bank(item_def->type, item_def->name)) {
        return CommandResult::with_msg(msgs_.bank_full);
    }
    player.remove_inventory_item(static_cast<uint8_t>(slot_opt->slot_index));

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Depositaste " + item_def->name};
    return {.private_events = {msg, inv_ev, make_bank_update_event(player)}};
}

CommandResult BankService::handle_bank_withdraw(uint16_t player_id, const BankWithdrawCmd& cmd) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;

    if (player.is_dead()) {
        return CommandResult::with_msg(msgs_.ghost_cant_withdraw);
    }

    auto map_it = maps_.find(player.get_current_map());
    if (map_it == maps_.end())
        return {};

    const Map& map = map_it->second;
    const int range = map.tile_size() * balance_.merchant.interaction_range_tiles;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    if (!map.prop_grid().is_in_range_of("banquero", px, py, range)) {
        return CommandResult::with_msg(msgs_.no_banker_nearby);
    }

    if (cmd.is_gold) {
        if (cmd.gold_amount == 0 || !player.take_bank_gold(cmd.gold_amount)) {
            return CommandResult::with_msg(msgs_.insufficient_bank_gold);
        }
        player.gain_gold(cmd.gold_amount);

        GoldUpdateEvent gold_ev{player.get_gold()};
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "Retiraste " + std::to_string(cmd.gold_amount) + " de oro"};
        return {.private_events = {msg, gold_ev, make_bank_update_event(player)}};
    }

    const Item* item_def = item_catalog_.find_by_name(cmd.item_name);
    if (!item_def) {
        return CommandResult::with_msg(
                std::vformat(msgs_.item_not_found, std::make_format_args(cmd.item_name)));
    }

    auto slot_opt = player.find_bank_slot_by_type(item_def->type);
    if (!slot_opt) {
        return CommandResult::with_msg(
                std::vformat(msgs_.item_not_in_bank, std::make_format_args(item_def->name)));
    }

    if (!player.add_item(item_def->type, item_def->name)) {
        return CommandResult::with_msg(msgs_.inventory_full);
    }
    player.remove_bank_item(static_cast<uint8_t>(slot_opt->slot_index));

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Retiraste " + item_def->name};
    return {.private_events = {msg, inv_ev, make_bank_update_event(player)}};
}
