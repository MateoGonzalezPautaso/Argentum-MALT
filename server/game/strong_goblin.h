#ifndef STRONG_GOBLIN_H
#define STRONG_GOBLIN_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class StrongGoblin: public EnemyNpc {
public:
    StrongGoblin(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
