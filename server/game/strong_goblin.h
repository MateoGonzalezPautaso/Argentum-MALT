#ifndef STRONG_GOBLIN_H
#define STRONG_GOBLIN_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// More dangerous variant of WeakGoblin
class StrongGoblin: public EnemyNpc {
public:
    StrongGoblin(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level);
};

#endif
