#ifndef SERVER_CLAN_RECORD_H
#define SERVER_CLAN_RECORD_H

#include <cstdint>
#include <cstring>
#include <string>

struct ClanRecord {
    static constexpr std::size_t NAME_MAX = 32;
    static constexpr std::size_t USERNAME_MAX = 32;
    static constexpr std::size_t MAX_MEMBERS = 16;

    char clan_name[NAME_MAX];
    char founder_username[USERNAME_MAX];
    uint8_t member_count;
    char members[MAX_MEMBERS][USERNAME_MAX];
    uint8_t banned_count;
    char banned[MAX_MEMBERS][USERNAME_MAX];

    ClanRecord();

    void set_clan_name(const std::string& name);
    std::string get_clan_name() const;
    void set_founder(const std::string& username);
    std::string get_founder() const;
};

#endif
