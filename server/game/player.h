#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/item_catalog.h"
#include "../../common/messages.h"
#include "../core/config.h"

#include "entity.h"
#include "inventory.h"
#include "player_inventory.h"

struct UpdateStats {
    uint32_t mana_max;
    uint32_t hp_max;
    uint32_t strength;
    uint32_t agility;
};

class Player: public Entity {
private:
    uint16_t id;
    Race race;
    PlayerClass player_class;
    uint32_t experience;
    uint32_t mana_current;
    uint32_t mana_max;
    uint32_t gold;
    bool is_meditating = false;
    BalanceConfig balance;
    bool cheat_infinite_hp = false;
    bool cheat_infinite_mana = false;
    bool cheat_fast_velocity = false;
    std::string clan_name;
    PlayerInventory inv;
    InventorySlot equipped[EQUIP_SLOT_COUNT];
    uint32_t strength;
    uint32_t agility;
    Inventory bank_inv;
    uint32_t bank_gold;

    UpdateStats update_stats() const;

public:
    Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
           PlayerClass player_class, const BalanceConfig& balance, uint8_t equip_capacity,
           uint8_t hp_potion_capacity, uint8_t mana_potion_capacity, uint8_t bank_capacity);

    Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
           PlayerClass player_class, const BalanceConfig& balance, uint8_t level,
           uint32_t experience, uint32_t hp_current, uint32_t hp_max, uint32_t mana_current,
           uint32_t mana_max, uint32_t gold, uint8_t equip_capacity, uint8_t hp_potion_capacity,
           uint8_t mana_potion_capacity, uint8_t bank_capacity, uint32_t bank_gold);

    uint16_t get_id() const { return id; }
    Race get_race() const { return race; }
    PlayerClass get_player_class() const { return player_class; }

    uint32_t get_experience() const { return experience; }
    uint32_t get_mana_current() const { return mana_current; }
    uint32_t get_mana_max() const { return mana_max; }
    uint32_t get_gold() const { return gold; }
    uint32_t get_strength() const { return strength; }
    uint32_t get_agility() const { return agility; }

    const std::string& get_clan_name() const { return clan_name; }
    void set_clan_name(const std::string& name) { clan_name = name; }

    bool get_is_meditating() const { return is_meditating; }
    void set_meditating(bool val) { is_meditating = val; }

    bool toggle_cheat_infinite_hp() { return cheat_infinite_hp = !cheat_infinite_hp; }
    bool toggle_cheat_infinite_mana() { return cheat_infinite_mana = !cheat_infinite_mana; }
    bool toggle_cheat_fast_velocity() { return cheat_fast_velocity = !cheat_fast_velocity; }
    bool has_cheat_fast_velocity() const { return cheat_fast_velocity; }
    void level_down();

    void apply_move(Direction new_dir, int dx, int dy);
    void resurrect();
    void gain_experience(uint32_t exp);
    void lose_experience_on_death();
    void level_up();
    void take_damage(uint32_t damage) override;
    void heal(uint32_t amount);
    void restore_mana(uint32_t amount);
    void use_mana(uint32_t amount);
    void gain_gold(uint32_t amount);
    void spend_gold(uint32_t amount);
    uint32_t take_excess_gold();

    uint32_t get_bank_gold() const { return bank_gold; }
    void add_bank_gold(uint32_t amount) { bank_gold += amount; }
    bool take_bank_gold(uint32_t amount);

    std::vector<InventorySlot> dump_bank() const;
    bool add_to_bank(ItemType type, const std::string& name);
    void remove_bank_item(uint8_t slot_index);
    std::vector<InventorySlotRecord> dump_bank_records() const;
    void load_bank(const std::vector<InventorySlotRecord>& records);

    uint32_t exp_to_next_level() const;

    void increase_max_hp(uint32_t amount);
    void increase_max_mana(uint32_t amount);

    std::vector<InventorySlot> dump_inventory() const;
    void load_inventory(const std::vector<InventorySlotRecord>& records);
    bool add_item(ItemType type, const std::string& name);
    void remove_inventory_item(uint8_t slot_index);
    void clear_inventory();
    std::vector<InventorySlotRecord> dump_inventory_records() const;

    bool equip(uint8_t inv_slot_index, const ItemCatalog& catalog);
    void unequip(EquipSlot eslot);
    const InventorySlot& get_equipped(EquipSlot eslot) const;
    void dump_equipped(InventorySlot out[EQUIP_SLOT_COUNT]) const;
    void restore_equipment(const InventorySlot equipped[EQUIP_SLOT_COUNT]);
    void clear_equipped();
};

#endif  // PLAYER_H
