#ifndef IRON_GOLEM_H
#define IRON_GOLEM_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Game's hardest enemy
class IronGolem: public EnemyNpc {
public:
    IronGolem(Position position, Rng& rng, EquipableItems& equipable_items, uint32_t level);
};

#endif
