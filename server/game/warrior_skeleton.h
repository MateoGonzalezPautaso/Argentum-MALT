#ifndef WARRIOR_SKELETON_H
#define WARRIOR_SKELETON_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class WarriorSkeleton: public EnemyNpc {
public:
    WarriorSkeleton(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level);
};

#endif
