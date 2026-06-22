#include "clan_command_handler.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

CommandResult system_msg(const std::string& msg) {
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = {}};
}

}  // namespace

ClanCommandHandler::ClanCommandHandler(
        ClanManager& clan_manager, std::map<uint16_t, Player>& players,
        const std::unordered_map<std::string, uint16_t>& player_name_index):
        clan_manager(clan_manager), players(players), player_name_index_(player_name_index) {}

std::optional<uint16_t> ClanCommandHandler::find_player_id_by_name(const std::string& name) const {
    auto it = player_name_index_.find(name);
    if (it == player_name_index_.end())
        return std::nullopt;
    return it->second;
}

std::optional<CommandResult> ClanCommandHandler::handle(uint16_t player_id,
                                                        const std::string& cmd_name,
                                                        const std::string& args) {
    if (cmd_name == "/c" || cmd_name == "/clan")
        return handle_clan_chat(player_id, args);
    return std::nullopt;
}

CommandResult ClanCommandHandler::notify_clan_members(const std::string& clan_name,
                                                      const ClanNotificationEvent& notif,
                                                      uint16_t exclude_id) {
    CommandResult result;
    for (const auto& [pid, p]: players) {
        if (pid == exclude_id)
            continue;
        if (p.get_clan_name() == clan_name)
            result.targeted_events[pid].push_back(notif);
    }
    return result;
}

CommandResult ClanCommandHandler::handle_found_clan(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /fundar-clan <nombre>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    if (it->second.get_level() < clan_manager.min_level_found()) {
        return system_msg("Necesitas nivel " + std::to_string(clan_manager.min_level_found()) +
                          " para fundar un clan");
    }

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.create_clan(sender_name, args);
    CommandResult cmd_result = system_msg(result.error_msg);
    if (result.ok) {
        it->second.set_clan_name(args);
        send_clan_update(args, cmd_result.targeted_events);
    }
    return cmd_result;
}

CommandResult ClanCommandHandler::handle_join_clan(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /unirse <nombre del clan>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.request_join(sender_name, args);

    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    if (result.ok) {
        auto members = clan_manager.get_member_list(args);
        auto founder_it = std::find_if(members.begin(), members.end(),
                                       [](const auto& m) { return m.is_founder; });
        if (founder_it != members.end()) {
            ClanNotificationEvent notif{ClanNotifType::JOIN_REQUEST, sender_name, args};
            auto founder_id_opt = find_player_id_by_name(founder_it->username);
            if (founder_id_opt)
                targeted[*founder_id_opt].push_back(notif);
        }
    }
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", result.error_msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
}

CommandResult ClanCommandHandler::handle_clan_status(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    if (!clan_manager.is_in_clan(sender_name))
        return system_msg("No perteneces a ningun clan");

    if (!clan_manager.is_founder(sender_name))
        return system_msg("Solo el fundador puede revisar el clan");

    std::string clan_name = clan_manager.get_clan_name(sender_name);
    auto members = clan_manager.get_member_list(clan_name);
    for (auto& m: members) {
        m.is_online = find_player_id_by_name(m.username).has_value();
    }
    auto requests = clan_manager.get_pending_requests(clan_name);

    std::string msg = "--- Clan: " + clan_name + " ---\nMiembros:";
    for (const auto& m: members) {
        msg += "\n  " + m.username;
        if (m.is_founder)
            msg += " (fundador)";
        msg += m.is_online ? " [En linea]" : " [Desconectado]";
        msg += "\n";
    }
    if (!requests.empty()) {
        msg += "\nPedidos pendientes:";
        for (const auto& r: requests) msg += "\n  " + r;
    } else {
        msg += "\nNo hay pedidos pendientes";
    }
    return system_msg(msg);
}

CommandResult ClanCommandHandler::handle_clan_accept(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /clan-aceptar <nick>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.accept_member(sender_name, args);

    CommandResult aresult;
    aresult.private_events = {ChatMsgEvent{ChatMsgType::SYSTEM, "", result.error_msg}};
    if (result.ok) {
        std::string clan_name = clan_manager.get_clan_name(sender_name);
        auto target_id_opt = find_player_id_by_name(args);
        if (target_id_opt) {
            auto pit = players.find(*target_id_opt);
            if (pit != players.end())
                pit->second.set_clan_name(clan_name);
        }
        send_clan_update(clan_name, aresult.targeted_events);
    }
    return aresult;
}

CommandResult ClanCommandHandler::handle_clan_reject(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /clan-rechazar <nick>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.reject_member(sender_name, args);

    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    if (result.ok) {
        std::string clan_name = clan_manager.get_clan_name(sender_name);
        ClanNotificationEvent notif{ClanNotifType::JOIN_REJECTED, args, clan_name};
        auto target_id_opt = find_player_id_by_name(args);
        if (target_id_opt)
            targeted[*target_id_opt].push_back(notif);
    }
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", result.error_msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
}

CommandResult ClanCommandHandler::handle_clan_ban(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /clan-ban <nick>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.ban_member(sender_name, args);

    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    if (result.ok) {
        std::string clan_name = clan_manager.get_clan_name(sender_name);
        auto target_id_opt = find_player_id_by_name(args);
        if (target_id_opt) {
            uint16_t target_id = *target_id_opt;
            auto pit = players.find(target_id);
            if (pit != players.end())
                pit->second.set_clan_name("");
            targeted[target_id].push_back(
                    ClanNotificationEvent{ClanNotifType::KICKED, args, clan_name});
            send_empty_clan_update(target_id, targeted);
        }
        send_clan_update(clan_name, targeted);
    }
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", result.error_msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
}

CommandResult ClanCommandHandler::handle_clan_unban(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /clan-unban <nick>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.unban_member(sender_name, args);

    return system_msg(result.error_msg);
}

CommandResult ClanCommandHandler::handle_clan_kick(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /clan-kick <nick>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& sender_name = it->second.get_name();
    ClanResult result = clan_manager.kick_member(sender_name, args);

    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    if (result.ok) {
        std::string clan_name = clan_manager.get_clan_name(sender_name);
        auto target_id_opt = find_player_id_by_name(args);
        if (target_id_opt) {
            uint16_t target_id = *target_id_opt;
            auto pit = players.find(target_id);
            if (pit != players.end())
                pit->second.set_clan_name("");
            targeted[target_id].push_back(
                    ClanNotificationEvent{ClanNotifType::KICKED, args, clan_name});
            send_empty_clan_update(target_id, targeted);
        }
        send_clan_update(clan_name, targeted);
    }
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", result.error_msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
}

CommandResult ClanCommandHandler::handle_leave_clan(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    std::string clan_name = it->second.get_clan_name();
    ClanResult result = clan_manager.leave_clan(it->second.get_name());
    CommandResult cmd_result;
    if (result.ok) {
        it->second.set_clan_name("");
        cmd_result.private_events = {ChatMsgEvent{ChatMsgType::SYSTEM, "", result.error_msg}};
        send_empty_clan_update(player_id, cmd_result.targeted_events);
        send_clan_update(clan_name, cmd_result.targeted_events);
    } else {
        cmd_result = system_msg(result.error_msg);
    }
    return cmd_result;
}

CommandResult ClanCommandHandler::handle_clan_chat(uint16_t player_id, const std::string& args) {
    if (args.empty())
        return system_msg("Uso: /c <mensaje>");

    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& clan_name = it->second.get_clan_name();
    if (clan_name.empty())
        return system_msg("No perteneces a ningun clan");

    ChatMsgEvent clan_msg{ChatMsgType::CLAN, it->second.get_name(), args};
    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    for (const auto& [pid, p]: players) {
        if (p.get_clan_name() == clan_name)
            targeted[pid].push_back(clan_msg);
    }
    return {.private_events = {}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
}

void ClanCommandHandler::send_clan_update(const std::string& clan_name,
                                          std::map<uint16_t, std::vector<ServerEvent>>& targeted) {
    auto members = clan_manager.get_member_list(clan_name);
    std::vector<ClanMember> online_members;
    for (auto& m: members) {
        auto id_opt = find_player_id_by_name(m.username);
        m.is_online = id_opt.has_value() && players.count(*id_opt) > 0 &&
                      players.at(*id_opt).get_clan_name() == clan_name;
        online_members.push_back(m);
    }
    ClanUpdateEvent ev{clan_name, online_members};
    for (const auto& [pid, p]: players) {
        if (p.get_clan_name() == clan_name)
            targeted[pid].push_back(ev);
    }
}

void ClanCommandHandler::send_empty_clan_update(
        uint16_t player_id, std::map<uint16_t, std::vector<ServerEvent>>& targeted) {
    ClanUpdateEvent ev{"", {}};
    targeted[player_id].push_back(ev);
}
