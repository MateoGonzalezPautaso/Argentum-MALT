#include "basic_skeleton.h"

#define MAX_HP_BASIC_SKELETON 175
#define DAMAGE_BASIC_SKELETON 10

BasicSkeleton::BasicSkeleton(Position position, Rng& rng, ItemCatalog& catalog,
                             uint32_t level):
        EnemyNpc(position, MAX_HP_BASIC_SKELETON * level, DAMAGE_BASIC_SKELETON * level, rng,
                 catalog, level) {}
