#include "warrior_skeleton.h"

#define MAX_HP_WAR_SKELETON 425
#define DAMAGE_WARRIOR_SKELETON 22

WarriorSkeleton::WarriorSkeleton(Position position, Rng& rng, ItemCatalog& catalog,
                                 uint32_t level):
        EnemyNpc(position, MAX_HP_WAR_SKELETON * level, DAMAGE_WARRIOR_SKELETON * level, rng,
                 catalog, level) {}
