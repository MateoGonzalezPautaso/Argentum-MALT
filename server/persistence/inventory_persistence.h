#ifndef SERVER_INVENTORY_PERSISTENCE_H
#define SERVER_INVENTORY_PERSISTENCE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "inventory_record.h"

class InventoryPersistence {
private:
    std::string data_path;
    std::string index_path;
    uint8_t slot_count;
    std::unordered_map<std::string, uint32_t> index;

    void load_index();
    bool save_index();
    uint32_t blob_size() const;

    static void log_error(const std::string& msg);

public:
    InventoryPersistence(const std::string& data_path, const std::string& index_path,
                         uint8_t slot_count);

    bool load(const std::string& username, std::vector<InventorySlotRecord>& out);
    bool save(const std::string& username, const std::vector<InventorySlotRecord>& records);
};

#endif  // SERVER_INVENTORY_PERSISTENCE_H
