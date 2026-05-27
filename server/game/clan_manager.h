#ifndef CLAN_MANAGER_H
#define CLAN_MANAGER_H

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "../../common/messages.h"
#include "../persistence/clan_persistence.h"

struct ClanResult {
    bool ok;
    std::string error_msg;
};

class ClanManager {
public:
    static constexpr size_t MAX_MEMBERS = 16;
    static constexpr uint8_t MIN_LEVEL_FOUND = 6;

    explicit ClanManager(ClanPersistence& persistence);

    ClanResult create_clan(const std::string& founder_username, const std::string& clan_name);
    ClanResult request_join(const std::string& applicant_username, const std::string& clan_name);
    ClanResult accept_member(const std::string& founder_username, const std::string& target_username);
    ClanResult reject_member(const std::string& founder_username, const std::string& target_username);
    ClanResult ban_member(const std::string& founder_username, const std::string& target_username);
    ClanResult kick_member(const std::string& founder_username, const std::string& target_username);
    ClanResult leave_clan(const std::string& username);

    bool is_in_clan(const std::string& username) const;
    bool is_founder(const std::string& username) const;
    bool are_in_same_clan(const std::string& u1, const std::string& u2) const;
    std::string get_clan_name(const std::string& username) const;
    std::vector<ClanMember> get_member_list(const std::string& clan_name) const;
    std::vector<std::string> get_pending_requests(const std::string& clan_name) const;
    bool has_pending_request(const std::string& username, const std::string& clan_name) const;

private:
    struct ClanData {
        std::string name;
        std::string founder;
        std::vector<std::string> members;
        std::vector<std::string> join_requests;
        std::unordered_set<std::string> banned;
    };

    std::map<std::string, ClanData> clans;
    std::map<std::string, std::string> player_to_clan;
    ClanPersistence& persistence;

    ClanData* find_clan_by_member(const std::string& username);
    const ClanData* find_clan_by_name(const std::string& clan_name) const;
    void add_member_to_clan(ClanData& clan, const std::string& username);
    void save_clan(const ClanData& clan);
};

#endif
