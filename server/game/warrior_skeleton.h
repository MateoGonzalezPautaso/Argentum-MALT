#ifndef WARRIOR_SKELETON_H
#define WARRIOR_SKELETON_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class WarriorSkeleton: public EnemyNpc {
public:
    WarriorSkeleton(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
