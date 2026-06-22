#ifndef CLAN_COMMAND_HANDLER_H
#define CLAN_COMMAND_HANDLER_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../common/messages.h"

#include "clan_manager.h"
#include "command_result.h"
#include "player.h"

class ClanCommandHandler {
public:
    ClanCommandHandler(ClanManager& clan_manager, std::map<uint16_t, Player>& players,
                       const std::unordered_map<std::string, uint16_t>& player_name_index);

    std::optional<CommandResult> handle(uint16_t player_id, const std::string& cmd_name,
                                        const std::string& args);

    CommandResult notify_clan_members(const std::string& clan_name,
                                      const ClanNotificationEvent& notif, uint16_t exclude_id = 0);

    CommandResult handle_found_clan(uint16_t player_id, const std::string& clan_name);
    CommandResult handle_join_clan(uint16_t player_id, const std::string& clan_name);
    CommandResult handle_clan_status(uint16_t player_id);
    CommandResult handle_clan_accept(uint16_t player_id, const std::string& target_nick);
    CommandResult handle_clan_reject(uint16_t player_id, const std::string& target_nick);
    CommandResult handle_clan_ban(uint16_t player_id, const std::string& target_nick);
    CommandResult handle_clan_unban(uint16_t player_id, const std::string& target_nick);
    CommandResult handle_clan_kick(uint16_t player_id, const std::string& target_nick);
    CommandResult handle_leave_clan(uint16_t player_id);

private:
    ClanManager& clan_manager;
    std::map<uint16_t, Player>& players;
    const std::unordered_map<std::string, uint16_t>& player_name_index_;

    std::optional<uint16_t> find_player_id_by_name(const std::string& name) const;
    Player* require_player(uint16_t player_id);
    CommandResult apply_removal(uint16_t player_id, const std::string& args,
                                const std::string& usage_msg,
                                ClanResult (ClanManager::*action)(const std::string&,
                                                                   const std::string&));

    CommandResult handle_clan_chat(uint16_t player_id, const std::string& args);
    void send_clan_update(const std::string& clan_name,
                          std::map<uint16_t, std::vector<ServerEvent>>& targeted);
    void send_empty_clan_update(uint16_t player_id,
                                std::map<uint16_t, std::vector<ServerEvent>>& targeted);
};

#endif  // CLAN_COMMAND_HANDLER_H
