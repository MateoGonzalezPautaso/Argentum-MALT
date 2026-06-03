#ifndef BASIC_SKELETON_H
#define BASIC_SKELETON_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"
class BasicSkeleton: public EnemyNpc {
public:
    BasicSkeleton(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
