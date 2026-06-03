#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <string>

#include "../../common/item_catalog.h"
#include "../../common/messages.h"
#include "../core/config.h"
#include "inventory.h"

struct UpdateStats {
    uint32_t mana_max;
    uint32_t hp_max;
    uint32_t strength;
};

class Player {
private:
    uint16_t id;
    std::string username;
    Position pos;
    Direction dir;
    Race race;
    PlayerClass player_class;
    uint8_t level;
    uint32_t experience;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t mana_current;
    uint32_t mana_max;
    uint32_t gold;
    uint32_t next_attack_tick = 0;
    bool is_meditating = false;
    BalanceConfig balance;
    bool cheat_infinite_hp = false;
    bool cheat_infinite_mana = false;
    bool cheat_fast_velocity = false;
    std::string clan_name;
    std::string current_map = "main";
    Inventory inventory;
    InventorySlot equipped[EQUIP_SLOT_COUNT];
    uint32_t strength;

    UpdateStats update_stats() const;

public:
    Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
           PlayerClass player_class, const BalanceConfig& balance, uint8_t inv_capacity);

    Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
           PlayerClass player_class, const BalanceConfig& balance,
           uint8_t level, uint32_t experience,
           uint32_t hp_current, uint32_t hp_max,
           uint32_t mana_current, uint32_t mana_max,
           uint32_t gold, uint8_t inv_capacity, uint32_t strength);

    uint16_t get_id() const { return id; }
    const std::string& get_username() const { return username; }
    Position get_pos() const { return pos; }
    uint16_t pos_x() const { return pos.x; }
    uint16_t pos_y() const { return pos.y; }
    Direction get_dir() const { return dir; }
    Race get_race() const { return race; }
    PlayerClass get_player_class() const { return player_class; }

    uint8_t get_level() const { return level; }
    uint32_t get_experience() const { return experience; }
    uint32_t get_hp_current() const { return hp_current; }
    uint32_t get_hp_max() const { return hp_max; }
    uint32_t get_mana_current() const { return mana_current; }
    uint32_t get_mana_max() const { return mana_max; }
    uint32_t get_gold() const { return gold; }
    uint32_t get_strength() const { return strength; }

    const std::string& get_clan_name() const { return clan_name; }
    void set_clan_name(const std::string& name) { clan_name = name; }

    const std::string& get_current_map() const { return current_map; }
    void set_current_map(const std::string& m) { current_map = m; }

    bool get_is_meditating() const { return is_meditating; }
    void set_meditating(bool val) { is_meditating = val; }

    bool toggle_cheat_infinite_hp() { return cheat_infinite_hp = !cheat_infinite_hp; }
    bool toggle_cheat_infinite_mana() { return cheat_infinite_mana = !cheat_infinite_mana; }
    bool toggle_cheat_fast_velocity() { return cheat_fast_velocity = !cheat_fast_velocity; }
    bool has_cheat_fast_velocity() const { return cheat_fast_velocity; }
    void kill() { hp_current = 0; }
    void level_down();

    bool try_attack(uint32_t current_tick, uint32_t cooldown_ticks);
    bool is_dead() const;

    void apply_move(Direction new_dir, int dx, int dy);
    void set_pos(uint16_t x, uint16_t y) { pos = {x, y}; }
    void resurrect();
    void gain_experience(uint32_t exp);
    void level_up();
    void take_damage(uint32_t damage);
    void heal(uint32_t amount);
    void restore_mana(uint32_t amount);
    void use_mana(uint32_t amount);
    void gain_gold(uint32_t amount);
    void spend_gold(uint32_t amount);

    uint32_t exp_to_next_level() const;

    void increase_max_hp(uint32_t amount);
    void increase_max_mana(uint32_t amount);

    std::vector<InventorySlot> dump_inventory() const;
    void load_inventory(const std::vector<InventorySlotRecord>& records);
    bool add_item(ItemType type, const std::string& name);
    std::vector<InventorySlotRecord> dump_inventory_records() const;

    bool equip(uint8_t inv_slot_index, const ItemCatalog& catalog);
    void unequip(EquipSlot eslot);
    const InventorySlot& get_equipped(EquipSlot eslot) const;
    void dump_equipped(InventorySlot out[EQUIP_SLOT_COUNT]) const;
    void restore_equipment(const InventorySlot equipped[EQUIP_SLOT_COUNT]);
};

#endif  // PLAYER_H
