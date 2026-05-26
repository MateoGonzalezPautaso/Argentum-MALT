#ifndef ENEMY_NPC_H
#define ENEMY_NPC_H

#include <optional>

#include "../../common/equipable_items.h"
#include "../../common/messages.h"
#include "../../common/rng.h"

struct EnemyDrop {
    uint32_t gold;
    bool is_potion;
    bool is_object;
    std::optional<Item> item;
};


class EnemyNpc {
private:
    Position position;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t damage;
    Rng& rng;
    EquipableItems& equipable_items;

    uint32_t get_gold_reward();
    Item get_potion();

public:
    EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
             EquipableItems& equipable_items);
    void take_damage(uint32_t damage_taken);
    bool is_dead();
    EnemyDrop get_kill_reward();
    uint32_t get_damage() const;
};

#endif
