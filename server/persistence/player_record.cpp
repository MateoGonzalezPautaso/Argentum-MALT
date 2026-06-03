#include "player_record.h"

PlayerRecord::PlayerRecord() {
    std::memset(username, 0, sizeof(username));
    std::memset(password, 0, sizeof(password));
    pos_x = 0;
    pos_y = 0;
    dir = 0;
    race = 0;
    player_class = 0;
    level = 1;
    experience = 0;
    hp_current = 0;
    hp_max = 0;
    mana_current = 0;
    mana_max = 0;
    gold = 0;
    std::memset(current_map, 0, sizeof(current_map));
    std::memset(equipped_type, 0, sizeof(equipped_type));
    std::memset(equipped_name, 0, sizeof(equipped_name));
}

void PlayerRecord::set_current_map(const std::string& name) {
    std::strncpy(current_map, name.c_str(), MAP_NAME_MAX - 1);
    current_map[MAP_NAME_MAX - 1] = '\0';
}

std::string PlayerRecord::get_current_map() const {
    if (current_map[0] == '\0')
        return "main";
    return std::string(current_map, std::strlen(current_map));
}

void PlayerRecord::set_username(const std::string& name) {
    std::strncpy(username, name.c_str(), USERNAME_MAX - 1);
    username[USERNAME_MAX - 1] = '\0';
}

std::string PlayerRecord::get_username() const {
    return std::string(username, std::strlen(username));
}

void PlayerRecord::set_password(const std::string& pw) {
    std::strncpy(password, pw.c_str(), PASSWORD_MAX - 1);
    password[PASSWORD_MAX - 1] = '\0';
}

bool PlayerRecord::check_password(const std::string& pw) const {
    return std::strncmp(password, pw.c_str(), PASSWORD_MAX) == 0;
}
