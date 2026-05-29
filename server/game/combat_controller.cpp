#include "combat_controller.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "clan_manager.h"

CombatController::CombatController(const AttackConfig& config,
                                   std::map<uint16_t, Player>& players):
        config(config), players(players) {}

void CombatController::set_clan_manager(ClanManager& mgr) { clan_manager = &mgr; }

CommandResult CombatController::melee_attack(uint16_t attacker_id, uint16_t target_id,
                                              uint32_t current_tick) {
    auto attacker_it = players.find(attacker_id);
    if (attacker_it == players.end())
        return {};

    auto target_it = players.find(target_id);
    if (target_it == players.end())
        return {};

    if (attacker_id == target_id)
        return {};

    Player& attacker = attacker_it->second;
    Player& target = target_it->second;

    if (attacker.is_ghost() || target.is_ghost())
        return {};


    if (attacker.get_level() <= 12) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar siendo newbie"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    if (target.get_level() <= 12) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar a un jugador newbie"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    int level_diff = std::abs(static_cast<int>(attacker.get_level()) - static_cast<int>(target.get_level()));
    if (level_diff > 10) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No puedes atacar a un jugador con diferencia de niveles mayor a 10"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (clan_manager && !attacker.get_clan_name().empty() && !target.get_clan_name().empty() &&
        attacker.get_clan_name() == target.get_clan_name()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar a un miembro de tu clan"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    if (!in_range(attacker, target))
        return {};

    uint32_t damage = calculate_damage();

    // Clan proximity bonus: increase attack damage based on nearby clan members
    int nearby_allies = 0;
    if (clan_manager && !attacker.get_clan_name().empty()) {
        nearby_allies = count_nearby_clan_members(attacker);
        double bonus = std::min(CLAN_BONUS_PER_MEMBER * nearby_allies, CLAN_BONUS_MAX);
        damage = static_cast<uint32_t>(std::round(damage * (1.0 + bonus)));
    }

    target.take_damage(damage);

    DamageDealtEvent dealt{target.get_id(), damage};
    DamageReceivedEvent received{target.get_id(), attacker.get_id(), damage,
                                  target.get_hp_current(), target.get_hp_max()};
    ChatMsgEvent chat_msg{ChatMsgType::SYSTEM, "",
                           attacker.get_username() + " ataco a " + target.get_username() +
                                   " por " + std::to_string(damage) + " de daño"};
    std::vector<ServerEvent> broadcast = {received, chat_msg};

    if (target.is_ghost()) {
        EntityDiedEvent died{target_id};
        broadcast.push_back(died);
        attacker.gain_experience(static_cast<uint32_t>(config.xp_per_level_kill) *
                                  target.get_level());
    }

    // Notify clan members when someone is attacked
    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    if (clan_manager && !target.get_clan_name().empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_ATTACKED, target.get_username(),
                                    target.get_clan_name()};
        for (const auto& [pid, p]: players) {
            if (pid == attacker_id || pid == target_id)
                continue;
            if (p.get_clan_name() == target.get_clan_name()) {
                targeted[pid].push_back(notif);
            }
        }
    }

    return {.private_events = {dealt}, .broadcast_events = std::move(broadcast),
            .targeted_events = std::move(targeted)};
}

bool CombatController::in_range(const Player& attacker, const Player& target) const {
    const int dx = static_cast<int>(target.pos_x()) - static_cast<int>(attacker.pos_x());
    const int dy = static_cast<int>(target.pos_y()) - static_cast<int>(attacker.pos_y());
    const int dist_sq = dx * dx + dy * dy;
    const int range_sq = config.attack_range_px * config.attack_range_px;
    return dist_sq <= range_sq;
}

uint32_t CombatController::calculate_damage() const {
    int variance = 0;
    if (config.damage_variance > 0)
        variance = (std::rand() % (2 * config.damage_variance + 1)) - config.damage_variance;
    return static_cast<uint32_t>(std::max(1, config.base_damage + variance));
}

int CombatController::count_nearby_clan_members(const Player& player) const {
    if (!clan_manager)
        return 0;
    int count = 0;
    for (const auto& [pid, p]: players) {
        if (pid == player.get_id())
            continue;
        if (p.get_clan_name() != player.get_clan_name())
            continue;
        if (p.is_ghost())
            continue;
        const int dx = static_cast<int>(p.pos_x()) - static_cast<int>(player.pos_x());
        const int dy = static_cast<int>(p.pos_y()) - static_cast<int>(player.pos_y());
        const int dist_sq = dx * dx + dy * dy;
        if (dist_sq <= CLAN_BONUS_RANGE_PX * CLAN_BONUS_RANGE_PX) {
            ++count;
        }
    }
    return count;
}
