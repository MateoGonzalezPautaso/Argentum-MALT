#ifndef WARRIOR_SKELETON_H
#define WARRIOR_SKELETON_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Dungeon variant
class WarriorSkeleton: public EnemyNpc {
public:
    WarriorSkeleton(Position position, Rng& rng, EquipableItems& equipable_items, uint32_t level);
};

#endif
