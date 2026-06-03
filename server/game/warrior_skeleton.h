#ifndef WARRIOR_SKELETON_H
#define WARRIOR_SKELETON_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Dungeon variant
class WarriorSkeleton: public EnemyNpc {
public:
    WarriorSkeleton(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level);
};

#endif
