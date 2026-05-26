#include "clan_record.h"

#include <cstring>

ClanRecord::ClanRecord() {
    std::memset(clan_name, 0, sizeof(clan_name));
    std::memset(founder_username, 0, sizeof(founder_username));
    member_count = 0;
    std::memset(members, 0, sizeof(members));
    banned_count = 0;
    std::memset(banned, 0, sizeof(banned));
}

void ClanRecord::set_clan_name(const std::string& name) {
    std::strncpy(clan_name, name.c_str(), CLAN_NAME_MAX - 1);
    clan_name[CLAN_NAME_MAX - 1] = '\0';
}

std::string ClanRecord::get_clan_name() const {
    return std::string(clan_name, std::strlen(clan_name));
}

void ClanRecord::set_founder(const std::string& username) {
    std::strncpy(founder_username, username.c_str(), USERNAME_MAX - 1);
    founder_username[USERNAME_MAX - 1] = '\0';
}

std::string ClanRecord::get_founder() const {
    return std::string(founder_username, std::strlen(founder_username));
}
