#include "player_data_service.h"

#include <cstring>
#include <vector>

PlayerDataService::PlayerDataService(const std::string& data_dir, const ServerConfig& config):
        player_persistence(data_dir + "/players.dat", data_dir + "/players.idx"),
        inventory_persistence(data_dir + "/inventory.dat", data_dir + "/inventory.idx",
                              static_cast<uint8_t>(config.inventory.max_slots)),
        balance(config.balance),
        inv_capacity(static_cast<uint8_t>(config.inventory.max_slots)) {}

bool PlayerDataService::player_exists(const std::string& username) {
    PlayerRecord dummy;
    return player_persistence.load(username, dummy);
}

PlayerRecord PlayerDataService::load_record(const std::string& username) {
    PlayerRecord rec;
    player_persistence.load(username, rec);
    return rec;
}

std::optional<Player> PlayerDataService::load_player(uint16_t player_id,
                                                     const std::string& username,
                                                     const PlayerRecord& rec) {
    Player player(player_id, username, Position{rec.pos_x, rec.pos_y},
                  static_cast<Direction>(rec.dir), static_cast<Race>(rec.race),
                  static_cast<PlayerClass>(rec.player_class), balance, rec.level, rec.experience,
                  rec.hp_current, rec.hp_max, rec.mana_current, rec.mana_max, rec.gold,
                  inv_capacity, rec.strength, rec.agility);

    std::vector<InventorySlotRecord> inv_records;
    if (inventory_persistence.load(username, inv_records)) {
        player.load_inventory(inv_records);
    } else {
        inventory_persistence.save(username, player.dump_inventory_records());
    }

    InventorySlot rec_equipped[EQUIP_SLOT_COUNT];
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        rec_equipped[i].item_type = static_cast<ItemType>(rec.equipped_type[i]);
        rec_equipped[i].item_name =
                std::string(rec.equipped_name[i], std::strlen(rec.equipped_name[i]));
    }
    player.restore_equipment(rec_equipped);

    return player;
}

void PlayerDataService::save_player(const Player& player) {
    PlayerRecord rec;
    rec.set_username(player.get_name());
    rec.set_current_map(player.get_current_map());
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
    rec.strength = player.get_strength();
    rec.agility = player.get_agility();

    InventorySlot equipped[EQUIP_SLOT_COUNT];
    player.dump_equipped(equipped);
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        rec.equipped_type[i] = static_cast<uint8_t>(equipped[i].item_type);
        std::memset(rec.equipped_name[i], 0, PlayerRecord::EQUIPPED_NAME_MAX);
        std::strncpy(rec.equipped_name[i], equipped[i].item_name.c_str(),
                     PlayerRecord::EQUIPPED_NAME_MAX - 1);
    }

    player_persistence.save(player.get_name(), rec);

    inventory_persistence.save(player.get_name(), player.dump_inventory_records());
}

void PlayerDataService::save_new_player(const std::string& username, const PlayerRecord& record) {
    player_persistence.save(username, record);
}
