#include "player_persistence.h"

#include <cstring>
#include <fstream>
#include <utility>

#include "../game/player.h"
#include "endian_io.h"

PlayerPersistence::PlayerPersistence(const std::string& data_path, const std::string& index_path):
        data_path(data_path), index_path(index_path) {
    load_index();
}

void PlayerPersistence::load_index() {
    index.clear();

    std::ifstream idx(index_path, std::ios::binary);
    if (!idx.is_open()) {
        return;
    }

    while (idx.peek() != EOF) {
        uint32_t name_len = 0;
        if (!endian_io::read_u32_le(idx, name_len))
            break;

        std::string name(name_len, '\0');
        idx.read(name.data(), name_len);
        if (!idx)
            break;

        uint32_t offset = 0;
        if (!endian_io::read_u32_le(idx, offset))
            break;

        index.emplace(std::move(name), offset);
    }
}

void PlayerPersistence::save_index() {
    std::ofstream idx(index_path, std::ios::binary | std::ios::trunc);
    if (!idx.is_open()) {
        return;
    }

    for (const auto& [name, offset]: index) {
        uint32_t name_len = static_cast<uint32_t>(name.size());
        endian_io::write_u32_le(idx, name_len);
        idx.write(name.data(), name_len);
        endian_io::write_u32_le(idx, offset);
    }
}

bool PlayerPersistence::load(const std::string& username, PlayerRecord& out) {
    auto it = index.find(username);
    if (it == index.end()) {
        return false;
    }

    std::ifstream data(data_path, std::ios::binary);
    if (!data.is_open()) {
        return false;
    }

    data.seekg(it->second);
    if (!data) {
        return false;
    }

    return read_player_record(data, out);
}

void PlayerPersistence::save(const std::string& username, const PlayerRecord& record) {
    auto it = index.find(username);

    if (it != index.end()) {
        PlayerRecord previous;
        {
            std::ifstream in(data_path, std::ios::binary);
            if (in.is_open()) {
                in.seekg(it->second);
                read_player_record(in, previous);
            }
        }

        // Merge new record with existing one, preserving username and password
        PlayerRecord merged = record;
        std::memcpy(merged.username, previous.username, sizeof(merged.username));
        std::memcpy(merged.password, previous.password, sizeof(merged.password));

        std::ofstream data(data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!data.is_open()) {
            return;
        }
        data.seekp(it->second);
        write_player_record(data, merged);
    } else {
        // Append new record at the end
        std::fstream data(data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!data.is_open()) {
            std::ofstream create(data_path, std::ios::binary);
            data.open(data_path, std::ios::binary | std::ios::in | std::ios::out);
            if (!data.is_open())
                return;
        }
        data.seekp(0, std::ios::end);
        uint32_t offset = static_cast<uint32_t>(data.tellp());
        write_player_record(data, record);
        data.flush();

        index.emplace(username, offset);
        save_index();
    }
}

void PlayerPersistence::save(const Player& player) {
    PlayerRecord rec;
    load(player.get_username(), rec);
    rec.pos_x = player.pos_x();
    rec.pos_y = player.pos_y();
    rec.dir = static_cast<uint8_t>(player.get_dir());
    rec.race = static_cast<uint8_t>(player.get_race());
    rec.player_class = static_cast<uint8_t>(player.get_player_class());
    rec.level = player.get_level();
    rec.experience = player.get_experience();
    rec.hp_current = player.get_hp_current();
    rec.hp_max = player.get_hp_max();
    rec.mana_current = player.get_mana_current();
    rec.mana_max = player.get_mana_max();
    rec.gold = player.get_gold();
    rec.set_current_map(player.get_current_map());
    save(player.get_username(), rec);
}
