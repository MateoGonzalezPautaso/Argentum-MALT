#ifndef BASIC_SKELETON_H
#define BASIC_SKELETON_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Appear in intermediate zones
class BasicSkeleton: public EnemyNpc {
public:
    BasicSkeleton(Position position, Rng& rng, EquipableItems& equipable_items);
};

#endif
