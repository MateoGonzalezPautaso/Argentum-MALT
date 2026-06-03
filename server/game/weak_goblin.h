#ifndef WEAK_GOBLIN_H
#define WEAK_GOBLIN_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class WeakGoblin: public EnemyNpc {
public:
    WeakGoblin(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level);
};

#endif
