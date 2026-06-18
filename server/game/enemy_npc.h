#ifndef ENEMY_NPC_H
#define ENEMY_NPC_H

#include <optional>
#include <string>

#include "../../common/item_catalog.h"
#include "../../common/rng.h"
#include "../core/config.h"

#include "entity.h"

struct EnemyDrop {
    uint32_t gold;
    bool is_potion;
    bool is_object;
    std::optional<Item> item;
};

class EnemyNpc: public Entity {
private:
    uint32_t damage;
    Rng& rng;
    const ItemCatalog& catalog;

    uint32_t get_gold_reward();

public:
    EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
             const ItemCatalog& catalog, uint8_t level, const std::string& name,
             uint16_t sprite_id = 0);
    EnemyDrop get_kill_reward(const NpcDropConfig& drop_config);
    uint32_t get_damage() const;
};

#endif
