#include "inventory_persistence.h"

#include <fstream>
#include <utility>

#include "endian_io.h"

InventoryPersistence::InventoryPersistence(const std::string& data_path,
                                           const std::string& index_path, uint8_t slot_count):
        data_path(data_path), index_path(index_path), slot_count(slot_count) {
    load_index();
}

uint32_t InventoryPersistence::blob_size() const {
    return static_cast<uint32_t>(slot_count) * sizeof(InventorySlotRecord);
}

void InventoryPersistence::load_index() {
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

void InventoryPersistence::save_index() {
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

bool InventoryPersistence::load(const std::string& username,
                                std::vector<InventorySlotRecord>& out) {
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

    uint32_t bsize = blob_size();
    out.resize(slot_count);
    data.read(reinterpret_cast<char*>(out.data()), bsize);
    return static_cast<bool>(data);
}

void InventoryPersistence::save(const std::string& username,
                                const std::vector<InventorySlotRecord>& records) {
    auto it = index.find(username);

    if (it != index.end()) {
        std::ofstream data(data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!data.is_open()) {
            return;
        }
        data.seekp(it->second);
        data.write(reinterpret_cast<const char*>(records.data()), blob_size());
    } else {
        std::ofstream data(data_path,
                           std::ios::binary | std::ios::app | std::ios::in | std::ios::out);
        if (!data.is_open()) {
            return;
        }
        uint32_t offset = static_cast<uint32_t>(data.tellp());
        data.write(reinterpret_cast<const char*>(records.data()), blob_size());
        data.flush();

        index.emplace(username, offset);
        save_index();
    }
}
