#include "player_record.h"

#include <istream>
#include <ostream>

#include "endian_io.h"

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
    strength = 0;
    agility = 0;
    bank_gold = 0;
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
        return "city";
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

void write_player_record(std::ostream& os, const PlayerRecord& rec) {
    os.write(rec.username, PlayerRecord::USERNAME_MAX);
    os.write(rec.password, PlayerRecord::PASSWORD_MAX);
    endian_io::write_u16_le(os, rec.pos_x);
    endian_io::write_u16_le(os, rec.pos_y);
    os.put(static_cast<char>(rec.dir));
    os.put(static_cast<char>(rec.race));
    os.put(static_cast<char>(rec.player_class));
    os.put(static_cast<char>(rec.level));
    endian_io::write_u32_le(os, rec.experience);
    endian_io::write_u32_le(os, rec.hp_current);
    endian_io::write_u32_le(os, rec.hp_max);
    endian_io::write_u32_le(os, rec.mana_current);
    endian_io::write_u32_le(os, rec.mana_max);
    endian_io::write_u32_le(os, rec.gold);
    endian_io::write_u32_le(os, rec.bank_gold);
    os.write(rec.current_map, PlayerRecord::MAP_NAME_MAX);
    os.write(reinterpret_cast<const char*>(rec.equipped_type), PlayerRecord::EQUIPPED_SLOTS);
    for (std::size_t i = 0; i < PlayerRecord::EQUIPPED_SLOTS; ++i)
        os.write(rec.equipped_name[i], PlayerRecord::EQUIPPED_NAME_MAX);
}

bool read_player_record(std::istream& is, PlayerRecord& rec) {
    is.read(rec.username, PlayerRecord::USERNAME_MAX);
    is.read(rec.password, PlayerRecord::PASSWORD_MAX);
    if (!is)
        return false;
    if (!endian_io::read_u16_le(is, rec.pos_x))
        return false;
    if (!endian_io::read_u16_le(is, rec.pos_y))
        return false;

    char small[4];
    is.read(small, sizeof(small));
    if (!is)
        return false;
    rec.dir = static_cast<uint8_t>(small[0]);
    rec.race = static_cast<uint8_t>(small[1]);
    rec.player_class = static_cast<uint8_t>(small[2]);
    rec.level = static_cast<uint8_t>(small[3]);

    if (!endian_io::read_u32_le(is, rec.experience))
        return false;
    if (!endian_io::read_u32_le(is, rec.hp_current))
        return false;
    if (!endian_io::read_u32_le(is, rec.hp_max))
        return false;
    if (!endian_io::read_u32_le(is, rec.mana_current))
        return false;
    if (!endian_io::read_u32_le(is, rec.mana_max))
        return false;
    if (!endian_io::read_u32_le(is, rec.gold))
        return false;
    if (!endian_io::read_u32_le(is, rec.bank_gold))
        return false;

    is.read(rec.current_map, PlayerRecord::MAP_NAME_MAX);
    is.read(reinterpret_cast<char*>(rec.equipped_type), PlayerRecord::EQUIPPED_SLOTS);
    for (std::size_t i = 0; i < PlayerRecord::EQUIPPED_SLOTS; ++i)
        is.read(rec.equipped_name[i], PlayerRecord::EQUIPPED_NAME_MAX);
    return static_cast<bool>(is);
}
