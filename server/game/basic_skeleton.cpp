#include "basic_skeleton.h"

#define MAX_HP_BASIC_SKELETON 200
#define DAMAGE_BASIC_SKELETON 10

BasicSkeleton::BasicSkeleton(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_BASIC_SKELETON, DAMAGE_BASIC_SKELETON, rng, equipable_items) {}
