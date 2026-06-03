#ifndef WEAK_GOBLIN_H
#define WEAK_GOBLIN_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Initial enemy
class WeakGoblin: public EnemyNpc {
public:
    WeakGoblin(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level);
};

#endif
