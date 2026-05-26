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

    // Anti-friendly-fire: clan members cannot attack each other
    if (clan_manager && !attacker.clan_name.empty() && !target.clan_name.empty() &&
        attacker.clan_name == target.clan_name) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No puedes atacar a un miembro de tu clan"};
        return {.private_events = {msg}, .broadcast_events = {}};
    }

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    if (!in_range(attacker, target))
        return {};

    uint32_t damage = calculate_damage();

    // Clan proximity bonus: increase attack damage based on nearby clan members
    int nearby_allies = 0;
    if (clan_manager && !attacker.clan_name.empty()) {
        nearby_allies = count_nearby_clan_members(attacker);
        double bonus = std::min(CLAN_BONUS_PER_MEMBER * nearby_allies, CLAN_BONUS_MAX);
        damage = static_cast<uint32_t>(std::round(damage * (1.0 + bonus)));
    }

    target.take_damage(damage);

    DamageDealtEvent dealt{target.id, damage};
    DamageReceivedEvent received{target.id, attacker.id, damage, target.hp_current, target.hp_max};
    ChatMsgEvent chat_msg{ChatMsgType::SYSTEM, "",
                           attacker.username + " ataco a " + target.username +
                                   " por " + std::to_string(damage) + " de daño"};
    std::vector<ServerEvent> broadcast = {received, chat_msg};

    if (target.hp_current == 0) {
        EntityDiedEvent died{target_id};
        broadcast.push_back(died);
    }

    // Notify clan members when someone is attacked
    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    if (clan_manager && !target.clan_name.empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_ATTACKED, target.username,
                                    target.clan_name};
        for (const auto& [pid, p]: players) {
            if (pid == attacker_id || pid == target_id)
                continue;
            if (p.clan_name == target.clan_name) {
                targeted[pid].push_back(notif);
            }
        }
    }

    return {.private_events = {dealt}, .broadcast_events = std::move(broadcast),
            .targeted_events = std::move(targeted)};
}

bool CombatController::in_range(const Player& attacker, const Player& target) const {
    const int dx = static_cast<int>(target.pos_x()) - static_cast<int>(attacker.pos_x());
    const int dy = static_cast<int>(target.pos.y) - static_cast<int>(attacker.pos.y);
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
        if (pid == player.id)
            continue;
        if (p.clan_name != player.clan_name)
            continue;
        if (p.is_dead || p.is_ghost())
            continue;
        const int dx = static_cast<int>(p.pos.x) - static_cast<int>(player.pos.x);
        const int dy = static_cast<int>(p.pos.y) - static_cast<int>(player.pos.y);
        const int dist_sq = dx * dx + dy * dy;
        if (dist_sq <= CLAN_BONUS_RANGE_PX * CLAN_BONUS_RANGE_PX) {
            ++count;
        }
    }
    return count;
}
