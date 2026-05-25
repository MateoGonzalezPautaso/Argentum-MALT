#ifndef SERVER_PLAYER_PERSISTENCE_H
#define SERVER_PLAYER_PERSISTENCE_H

#include <cstdint>
#include <map>
#include <string>

#include "player_record.h"

class PlayerPersistence {
public:
    PlayerPersistence(const std::string& data_path, const std::string& index_path);

    // Returns true if the player was found in the index and loaded from the data file.
    bool load(const std::string& username, PlayerRecord& out);

    // Saves the record. If the username already exists, overwrites it in place;
    // otherwise appends a new record.
    void save(const std::string& username, const PlayerRecord& record);

private:
    std::string data_path;
    std::string index_path;
    std::map<std::string, uint32_t> index;

    void load_index();
    void save_index();
};

#endif  // SERVER_PLAYER_PERSISTENCE_H
