#include "basic_skeleton.h"

#define MAX_HP 175
#define DAMAGE 10
#define NAME "Basic skeleton"

BasicSkeleton::BasicSkeleton(Position position, Rng& rng, EquipableItems& equipable_items,
                             uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
