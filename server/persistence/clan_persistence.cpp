#include "clan_persistence.h"

#include <cstring>
#include <fstream>
#include <utility>

ClanPersistence::ClanPersistence(const std::string& data_path, const std::string& index_path):
        data_path(data_path), index_path(index_path) {
    load_index();
}

void ClanPersistence::load_index() {
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

void ClanPersistence::save_index() {
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

std::vector<ClanPersistence::ClanData> ClanPersistence::load_all() {
    std::vector<ClanData> result;

    for (const auto& [name, offset]: index) {
        std::ifstream data(data_path, std::ios::binary);
        if (!data.is_open())
            continue;

        data.seekg(offset);
        if (!data)
            continue;

        ClanRecord rec;
        data.read(reinterpret_cast<char*>(&rec), sizeof(ClanRecord));
        if (!data)
            continue;

        ClanData cd;
        cd.clan_name = rec.get_clan_name();
        cd.founder_username = rec.get_founder();

        for (uint8_t i = 0; i < rec.member_count && i < ClanRecord::MAX_MEMBERS; ++i) {
            cd.members.emplace_back(rec.members[i], std::strlen(rec.members[i]));
        }

        for (uint8_t i = 0; i < rec.banned_count && i < ClanRecord::MAX_MEMBERS; ++i) {
            cd.banned.emplace_back(rec.banned[i], std::strlen(rec.banned[i]));
        }

        result.push_back(std::move(cd));
    }

    return result;
}

void ClanPersistence::save_one(const std::string& clan_name, const ClanData& data) {
    ClanRecord rec;
    rec.set_clan_name(data.clan_name);
    rec.set_founder(data.founder_username);

    rec.member_count = static_cast<uint8_t>(data.members.size());
    for (uint8_t i = 0; i < rec.member_count && i < ClanRecord::MAX_MEMBERS; ++i) {
        std::strncpy(rec.members[i], data.members[i].c_str(), ClanRecord::USERNAME_MAX - 1);
        rec.members[i][ClanRecord::USERNAME_MAX - 1] = '\0';
    }

    rec.banned_count = static_cast<uint8_t>(data.banned.size());
    for (uint8_t i = 0; i < rec.banned_count && i < ClanRecord::MAX_MEMBERS; ++i) {
        std::strncpy(rec.banned[i], data.banned[i].c_str(), ClanRecord::USERNAME_MAX - 1);
        rec.banned[i][ClanRecord::USERNAME_MAX - 1] = '\0';
    }

    auto it = index.find(clan_name);

    if (it != index.end()) {
        std::ofstream data_file(data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!data_file.is_open())
            return;
        data_file.seekp(it->second);
        data_file.write(reinterpret_cast<const char*>(&rec), sizeof(ClanRecord));
    } else {
        std::ofstream data_file(data_path,
                                std::ios::binary | std::ios::app | std::ios::in | std::ios::out);
        if (!data_file.is_open())
            return;
        uint32_t offset = static_cast<uint32_t>(data_file.tellp());
        data_file.write(reinterpret_cast<const char*>(&rec), sizeof(ClanRecord));
        data_file.flush();
        index.emplace(clan_name, offset);
        save_index();
    }
}

void ClanPersistence::remove(const std::string& clan_name) {
    auto it = index.find(clan_name);
    if (it == index.end())
        return;
    index.erase(it);
    save_index();
}
