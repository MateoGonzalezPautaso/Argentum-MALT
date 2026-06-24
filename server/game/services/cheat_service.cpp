#include "cheat_service.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../combat_controller.h"

CheatService::CheatService(std::map<uint16_t, Player>& players, const BalanceConfig& balance,
                           const ItemCatalog& item_catalog, PlayerDataService& player_data_service,
                           CombatController& combat_controller,
                           const std::vector<std::string>& help_lines, bool cheats_enabled,
                           const MessagesConfig& msgs):
        players_(players),
        balance_(balance),
        item_catalog_(item_catalog),
        player_data_service_(player_data_service),
        combat_controller_(combat_controller),
        help_lines_(help_lines),
        cheats_enabled_(cheats_enabled),
        msgs_(msgs),
        handlers_({
                {CheatType::INFINITE_HP, &CheatService::handle_cheat_infinite_hp},
                {CheatType::INFINITE_MANA, &CheatService::handle_cheat_infinite_mana},
                {CheatType::DIE, &CheatService::handle_cheat_die},
                {CheatType::LEVEL_UP, &CheatService::handle_cheat_level_up},
                {CheatType::LEVEL_DOWN, &CheatService::handle_cheat_level_down},
                {CheatType::ADD_GOLD, &CheatService::handle_cheat_add_gold},
                {CheatType::RESET_GOLD, &CheatService::handle_cheat_reset_gold},
                {CheatType::VELOCITY, &CheatService::handle_cheat_velocity},
                {CheatType::REVIVE, &CheatService::handle_cheat_revive},
                {CheatType::FILL_INVENTORY, &CheatService::handle_cheat_fill_inventory},
                {CheatType::CLEAR_INVENTORY, &CheatService::handle_cheat_clear_inventory},
                {CheatType::RESET_MANA, &CheatService::handle_cheat_reset_mana},
        }) {}

CommandResult CheatService::dispatch(CheatType cheat_type, uint16_t player_id) {
    if (!cheats_enabled_)
        return {};
    auto it = handlers_.find(cheat_type);
    if (it == handlers_.end())
        return {};
    return (this->*it->second)(player_id);
}

PlayerStatsEvent CheatService::make_player_stats_event(const Player& p) const {
    PlayerStatsEvent ev{};
    combat_controller_.fill_player_stats_event(ev, p);
    return ev;
}

CommandResult CheatService::handle_cheat_infinite_hp(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    bool active = it->second.toggle_cheat_infinite_hp();
    return CommandResult::with_msg(active ? msgs_.cheat_inf_hp_on : msgs_.cheat_inf_hp_off);
}

CommandResult CheatService::handle_cheat_infinite_mana(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    bool active = player.toggle_cheat_infinite_mana();
    if (active)
        player.restore_mana(player.get_mana_max());
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? msgs_.cheat_inf_mana_on : msgs_.cheat_inf_mana_off};
    PlayerStatsEvent stats = make_player_stats_event(player);
    return {.private_events = {msg, stats}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_die(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    it->second.kill();
    DamageReceivedEvent dmg{.target_id = player_id,
                            .attacker_id = player_id,
                            .damage = 0,
                            .hp_current = 0,
                            .hp_max = it->second.get_hp_max()};
    EntityDiedEvent died{.entity_id = player_id};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.cheat_died};
    CommandResult r;
    r.private_events = {msg};
    r.map_events = {dmg, died};
    return r;
}

CommandResult CheatService::handle_cheat_revive(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    if (!player.is_dead()) {
        return CommandResult::with_msg(msgs_.cheat_not_dead);
    }
    player.resurrect();
    PlayerRespawnedEvent respawn_ev{player_id, player.get_hp_current(), player.get_hp_max()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.cheat_revived};
    CommandResult r;
    r.private_events = {msg};
    r.map_events = {respawn_ev};
    return r;
}

CommandResult CheatService::handle_cheat_level_up(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    if (player.get_level() >= balance_.max_level) {
        return CommandResult::with_msg(msgs_.cheat_max_level);
    }
    player.level_up();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel subido a " + std::to_string(player.get_level())};
    PlayerStatsEvent stats = make_player_stats_event(player);
    GoldUpdateEvent gold{player.get_gold()};
    return {.private_events = {msg, stats, gold}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_level_down(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    if (player.get_level() <= balance_.min_level) {
        return CommandResult::with_msg(msgs_.cheat_min_level);
    }
    player.level_down();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel bajado a " + std::to_string(player.get_level())};
    PlayerStatsEvent stats = make_player_stats_event(player);
    return {.private_events = {msg, stats}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_add_gold(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    player.gain_gold(static_cast<uint32_t>(balance_.cheat_gold_amount));
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] +" + std::to_string(balance_.cheat_gold_amount) +
                             " oro (total: " + std::to_string(player.get_gold()) + ")"};
    GoldUpdateEvent gold{player.get_gold()};
    return {.private_events = {msg, gold}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_reset_gold(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    player.spend_gold(player.get_gold());
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.cheat_gold_reset};
    GoldUpdateEvent gold{player.get_gold()};
    return {.private_events = {msg, gold}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_reset_mana(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    player.use_mana(player.get_mana_current());
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.cheat_mana_reset};
    PlayerStatsEvent stats = make_player_stats_event(player);
    return {.private_events = {msg, stats}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_velocity(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    bool active = player.toggle_cheat_fast_velocity();
    return CommandResult::with_msg(active ? msgs_.cheat_velocity_on : msgs_.cheat_velocity_off);
}

CommandResult CheatService::handle_cheat_fill_inventory(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    for (const auto& item: item_catalog_.all()) {
        if (item.type == ItemType::NONE || item.type == ItemType::GOLD_DROP)
            continue;
        player.add_item(item.type, item.name);
    }
    std::vector<InventorySlot> slots = player.dump_inventory();
    InventoryUpdateEvent inv{.slots = std::move(slots)};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.cheat_inventory_filled};
    player_data_service_.save_player(player);
    return {.private_events = {msg, inv}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_cheat_clear_inventory(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    player.clear_inventory();
    std::vector<InventorySlot> slots = player.dump_inventory();
    InventoryUpdateEvent inv{.slots = std::move(slots)};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.cheat_inventory_cleared};
    player_data_service_.save_player(player);
    return {.private_events = {msg, inv}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult CheatService::handle_help() {
    if (help_lines_.empty())
        return {};
    std::vector<ServerEvent> events;
    events.reserve(help_lines_.size());
    std::transform(help_lines_.begin(), help_lines_.end(), std::back_inserter(events),
                   [](const auto& line) {
                       return ChatMsgEvent{ChatMsgType::SYSTEM, "", line};
                   });
    return {.private_events = std::move(events)};
}
