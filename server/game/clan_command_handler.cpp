#include "clan_command_handler.h"

#include <string>
#include <utility>
#include <vector>

namespace {

CommandResult system_msg(const std::string& msg) {
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = {}};
}

}  // namespace

ClanCommandHandler::ClanCommandHandler(ClanManager& clan_manager,
                                       std::map<uint16_t, Player>& players):
        clan_manager(clan_manager), players(players) {}

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
    if (result.ok)
        it->second.set_clan_name(args);
    return system_msg(result.error_msg);
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
        for (const auto& m: members) {
            if (m.is_founder) {
                ClanNotificationEvent notif{ClanNotifType::JOIN_REQUEST, sender_name, args};
                for (const auto& [target_id, p]: players) {
                    if (p.get_name() == m.username) {
                        targeted[target_id].push_back(notif);
                        break;
                    }
                }
                break;
            }
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

    std::string clan_name = clan_manager.get_clan_name(sender_name);
    auto members = clan_manager.get_member_list(clan_name);
    for (auto& m: members) {
        m.is_online = false;
        for (const auto& [pid, p]: players) {
            if (p.get_name() == m.username) {
                m.is_online = true;
                break;
            }
        }
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
        for (auto& [pid, p]: players) {
            if (p.get_name() == args) {
                p.set_clan_name(clan_name);
                ClanNotificationEvent notif{ClanNotifType::JOIN_ACCEPTED, sender_name, clan_name};
                aresult.targeted_events[pid].push_back(notif);
                break;
            }
        }
        ClanNotificationEvent notif{ClanNotifType::MEMBER_ONLINE, args, clan_name};
        auto nresult = notify_clan_members(clan_name, notif, player_id);
        for (auto& [tid, tev]: nresult.targeted_events)
            for (auto& se: tev) aresult.targeted_events[tid].push_back(std::move(se));
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
        for (const auto& [pid, p]: players) {
            if (p.get_name() == args) {
                targeted[pid].push_back(notif);
                break;
            }
        }
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
        for (auto& [pid, p]: players) {
            if (p.get_name() == args) {
                p.set_clan_name("");
                ClanNotificationEvent notif{ClanNotifType::KICKED, args, clan_name};
                targeted[pid].push_back(notif);
                break;
            }
        }
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
        for (auto& [pid, p]: players) {
            if (p.get_name() == args) {
                p.set_clan_name("");
                ClanNotificationEvent notif{ClanNotifType::KICKED, args, clan_name};
                targeted[pid].push_back(notif);
                break;
            }
        }
    }
    ChatMsgEvent ev{ChatMsgType::SYSTEM, "", result.error_msg};
    return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = std::move(targeted)};
}

CommandResult ClanCommandHandler::handle_leave_clan(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    ClanResult result = clan_manager.leave_clan(it->second.get_name());
    if (result.ok)
        it->second.set_clan_name("");
    return system_msg(result.error_msg);
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
