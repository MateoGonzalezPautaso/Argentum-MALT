#ifndef CLAN_COMMAND_HANDLER_H
#define CLAN_COMMAND_HANDLER_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "../../common/messages.h"

#include "clan_manager.h"
#include "command_result.h"
#include "player.h"

class ClanCommandHandler {
public:
    ClanCommandHandler(ClanManager& clan_manager, std::map<uint16_t, Player>& players);

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

    CommandResult handle_clan_chat(uint16_t player_id, const std::string& args);
};

#endif  // CLAN_COMMAND_HANDLER_H
