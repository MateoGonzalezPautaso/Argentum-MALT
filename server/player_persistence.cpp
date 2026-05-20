#include "player_persistence.h"

#include <cstring>
#include <fstream>

PlayerPersistence::PlayerPersistence(const std::string& data_path,
                                     const std::string& index_path):
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
        idx.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (!idx)
            break;

        std::string name(name_len, '\0');
        idx.read(name.data(), name_len);
        if (!idx)
            break;

        uint32_t offset = 0;
        idx.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        if (!idx)
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
        idx.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        idx.write(name.data(), name_len);
        idx.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
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

    data.read(reinterpret_cast<char*>(&out), sizeof(PlayerRecord));
    return static_cast<bool>(data);
}

void PlayerPersistence::save(const std::string& username, const PlayerRecord& record) {
    auto it = index.find(username);

    if (it != index.end()) {
        // Overwrite existing record in place
        std::ofstream data(data_path,
                           std::ios::binary | std::ios::in | std::ios::out);
        if (!data.is_open()) {
            return;
        }
        data.seekp(it->second);
        data.write(reinterpret_cast<const char*>(&record), sizeof(PlayerRecord));
    } else {
        // Append new record at the end
        std::ofstream data(data_path,
                           std::ios::binary | std::ios::app | std::ios::in | std::ios::out);
        if (!data.is_open()) {
            return;
        }
        uint32_t offset = static_cast<uint32_t>(data.tellp());
        data.write(reinterpret_cast<const char*>(&record), sizeof(PlayerRecord));
        data.flush();

        index.emplace(username, offset);
        save_index();
    }
}
