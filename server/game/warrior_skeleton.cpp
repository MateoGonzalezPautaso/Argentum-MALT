#include "warrior_skeleton.h"

#define MAX_HP_WAR_SKELETON 500
#define DAMAGE_WARRIOR_SKELETON 22

WarriorSkeleton::WarriorSkeleton(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_WAR_SKELETON, DAMAGE_WARRIOR_SKELETON, rng, equipable_items) {}
