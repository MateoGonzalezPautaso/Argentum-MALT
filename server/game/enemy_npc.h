#ifndef ENEMY_NPC_H
#define ENEMY_NPC_H

#include <optional>

#include "../../common/item_catalog.h"
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
    ItemCatalog& catalog;
    uint32_t level;

    uint32_t get_gold_reward();

public:
    EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
             ItemCatalog& catalog, uint32_t level);
    void take_damage(uint32_t damage_taken);
    bool is_dead();
    EnemyDrop get_kill_reward();
    uint32_t get_damage() const;
    uint32_t get_level() const;
    uint32_t get_hp_max() const;
};

#endif
