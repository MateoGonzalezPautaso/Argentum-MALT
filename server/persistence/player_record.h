#ifndef SERVER_PLAYER_RECORD_H
#define SERVER_PLAYER_RECORD_H

#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <string>

// Binary structure for storing player data in the database.
struct PlayerRecord {
    static constexpr std::size_t USERNAME_MAX = 32;
    static constexpr std::size_t PASSWORD_MAX = 32;
    static constexpr std::size_t MAP_NAME_MAX = 32;
    static constexpr std::size_t EQUIPPED_SLOTS = 4;
    static constexpr std::size_t EQUIPPED_NAME_MAX = 32;

    char username[USERNAME_MAX];
    char password[PASSWORD_MAX];
    uint16_t pos_x;
    uint16_t pos_y;
    uint8_t dir;
    uint8_t race;
    uint8_t player_class;
    uint8_t level;
    uint32_t experience;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t mana_current;
    uint32_t mana_max;
    uint32_t gold;
    uint32_t strength;
    uint32_t agility;
    char current_map[MAP_NAME_MAX];

    uint8_t equipped_type[EQUIPPED_SLOTS];
    char equipped_name[EQUIPPED_SLOTS][EQUIPPED_NAME_MAX];

    PlayerRecord();

    void set_username(const std::string& name);
    std::string get_username() const;
    void set_password(const std::string& pw);
    bool check_password(const std::string& pw) const;
    void set_current_map(const std::string& name);
    std::string get_current_map() const;
};

void write_player_record(std::ostream& os, const PlayerRecord& rec);
bool read_player_record(std::istream& is, PlayerRecord& rec);

#endif  // SERVER_PLAYER_RECORD_H
