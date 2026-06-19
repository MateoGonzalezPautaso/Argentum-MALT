#ifndef SERVER_PLAYER_DATA_SERVICE_H
#define SERVER_PLAYER_DATA_SERVICE_H

#include <cstdint>
#include <optional>
#include <string>

#include "../core/config.h"
#include "../persistence/inventory_persistence.h"
#include "../persistence/player_persistence.h"

#include "player.h"

class PlayerDataService {
private:
    PlayerPersistence player_persistence;
    InventoryPersistence inventory_persistence;
    InventoryPersistence bank_persistence;
    const BalanceConfig& balance;
    uint8_t inv_capacity;
    uint8_t hp_potion_capacity;
    uint8_t mana_potion_capacity;
    uint8_t bank_capacity;

public:
    PlayerDataService(const std::string& data_dir, const ServerConfig& config);

    bool player_exists(const std::string& username);
    PlayerRecord load_record(const std::string& username);

    std::optional<Player> load_player(uint16_t player_id, const std::string& username,
                                      const PlayerRecord& rec);
    void save_player(const Player& player);

    void save_new_player(const std::string& username, const PlayerRecord& record);
};

#endif  // SERVER_PLAYER_DATA_SERVICE_H
