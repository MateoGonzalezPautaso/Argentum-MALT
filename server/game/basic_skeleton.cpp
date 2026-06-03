#include "basic_skeleton.h"

#define MAX_HP 175
#define DAMAGE 10
#define NAME "Basic skeleton"

BasicSkeleton::BasicSkeleton(Position position, Rng& rng, const ItemCatalog& catalog,
                             uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
