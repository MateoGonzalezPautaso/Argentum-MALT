#ifndef STRONG_GOBLIN_H
#define STRONG_GOBLIN_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// More dangerous variant of WeakGoblin
class StrongGoblin: public EnemyNpc {
public:
    StrongGoblin(Position position, Rng& rng, EquipableItems& equipable_items);
};

#endif
