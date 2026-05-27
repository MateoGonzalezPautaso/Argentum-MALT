#ifndef SERVER_CLAN_PERSISTENCE_H
#define SERVER_CLAN_PERSISTENCE_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "clan_record.h"

class ClanPersistence {
public:
    explicit ClanPersistence(const std::string& data_path, const std::string& index_path);

    struct ClanData {
        std::string clan_name;
        std::string founder_username;
        std::vector<std::string> members;
        std::vector<std::string> banned;
    };

    std::vector<ClanData> load_all();

    void save_one(const std::string& clan_name, const ClanData& data);

    void remove(const std::string& clan_name);

private:
    std::string data_path;
    std::string index_path;
    std::map<std::string, uint32_t> index;

    void load_index();
    void save_index();
};

#endif
