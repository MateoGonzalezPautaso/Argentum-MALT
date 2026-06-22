#include "spell_service.h"

#include "../../../common/error_logger.h"
#include "../combat_controller.h"
#include "../game_formulas.h"
#include "ground_item_service.h"

SpellService::SpellService(std::map<uint16_t, Player>& players,
                           const std::map<uint16_t, EnemyNpc>& enemy_npcs,
                           const std::unordered_map<std::string, Map>& maps,
                           const ItemCatalog& item_catalog, CombatController& combat_controller,
                           GroundItemService& ground_item_service, const BalanceConfig& balance,
                           const MessagesConfig& msgs):
        players_(players),
        enemy_npcs_(enemy_npcs),
        maps_(maps),
        item_catalog_(item_catalog),
        combat_controller_(combat_controller),
        ground_item_service_(ground_item_service),
        balance_(balance),
        msgs_(msgs) {}

const Map& SpellService::player_map(const Player& p) const {
    auto it = maps_.find(p.get_current_map());
    if (it == maps_.end()) {
        ErrorLogger::log("[SpellService] player_map: map '" + p.get_current_map() +
                         "' not found for player '" + p.get_name() + "' (id=" +
                         std::to_string(p.get_id()) + "), falling back to an arbitrary map");
        return maps_.begin()->second;
    }
    return it->second;
}

std::optional<CommandResult> SpellService::validate_cast(const Player& player,
                                                         const CastSpellCmd&) const {
    if (player.get_player_class() == PlayerClass::WARRIOR) {
        return CommandResult::with_msg(msgs_.warrior_no_magic);
    }

    const InventorySlot& weapon_slot = player.get_equipped(EquipSlot::WEAPON);
    if (weapon_slot.item_type == ItemType::NONE) {
        return CommandResult::with_msg(msgs_.no_weapon_equipped);
    }

    const Item* item = item_catalog_.find(weapon_slot.item_type);
    if (!item || item->mana_consumed == 0) {
        return CommandResult::with_msg(msgs_.weapon_not_magic);
    }

    if (player.get_mana_current() < item->mana_consumed) {
        return CommandResult::with_msg(msgs_.insufficient_mana);
    }

    const Map& map = player_map(player);
    if (map.is_safe_zone(player.pos_x(), player.pos_y())) {
        return CommandResult::with_msg(msgs_.spell_safe_zone);
    }

    return std::nullopt;
}

CommandResult SpellService::handle_cast_spell(uint16_t player_id, const CastSpellCmd& cmd,
                                              uint32_t tick_count) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};
    Player& player = it->second;
    if (player.is_dead())
        return {};

    if (auto rejection = validate_cast(player, cmd))
        return *rejection;

    const InventorySlot& weapon_slot = player.get_equipped(EquipSlot::WEAPON);
    const Item* item = item_catalog_.find(weapon_slot.item_type);

    player.use_mana(item->mana_consumed);
    player.set_meditating(false);

    if (item->self_heal) {
        uint32_t heal_amount = GameFormulas::spell_self_heal(player.get_hp_max());
        player.heal(heal_amount);
        PlayerStatsEvent stats{};
        combat_controller_.fill_player_stats_event(stats, player);
        HealReceivedEvent heal_ev{player_id, player.get_hp_current(), player.get_mana_current()};
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.self_heal_success};
        DamageDealtEvent spell_ev{player_id, 0};
        SpellEffectEvent effect_ev{player_id, 0};
        return {.private_events = {msg, heal_ev, stats, spell_ev},
                .broadcast_events = {},
                .targeted_events = {},
                .map_events = {effect_ev}};
    }

    if (cmd.target_id == player_id) {
        return CommandResult::with_msg(msgs_.attack_self);
    }

    auto target_player_it = players_.find(cmd.target_id);
    if (target_player_it != players_.end()) {
        const Player& target = target_player_it->second;
        auto target_map_it = maps_.find(target.get_current_map());
        if (target_map_it != maps_.end() &&
            target_map_it->second.is_safe_zone(target.pos_x(), target.pos_y())) {
            return CommandResult::with_msg(msgs_.spell_safe_zone);
        }
    } else {
        auto npc_it = enemy_npcs_.find(cmd.target_id);
        if (npc_it != enemy_npcs_.end()) {
            auto npc_map_it = maps_.find(npc_it->second.get_current_map());
            if (npc_map_it != maps_.end() &&
                npc_map_it->second.is_safe_zone(npc_it->second.pos_x(), npc_it->second.pos_y())) {
                return CommandResult::with_msg(msgs_.spell_safe_zone);
            }
        }
    }

    CommandResult result;
    if (enemy_npcs_.find(cmd.target_id) == enemy_npcs_.end())
        result = combat_controller_.spell_attack_player(player_id, cmd.target_id, tick_count);
    else
        result = combat_controller_.spell_attack_npc(player_id, cmd.target_id, tick_count);
    ground_item_service_.commit_ground_drops(result, result.ground_drops);
    result.map_events = std::move(result.broadcast_events);
    uint8_t effect_type = item->spell_effect_id;
    if (effect_type == 0)
        effect_type = balance_.default_spell_effect_id;
    result.map_events.push_back(SpellEffectEvent{cmd.target_id, effect_type});
    return result;
}
