#ifndef WEAK_GOBLIN_H
#define WEAK_GOBLIN_H

#include "../common/equipable_items.h"
#include "../common/rng.h"

#include "enemy_npc.h"

// Initial enemy
class WeakGoblin: public EnemyNpc {
public:
    WeakGoblin(Position position, Rng& rng, EquipableItems& equipable_items);
};

#endif
