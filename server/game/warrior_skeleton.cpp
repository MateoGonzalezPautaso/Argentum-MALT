#include "warrior_skeleton.h"

#define MAX_HP 425
#define DAMAGE 22
#define NAME "Warrior skeleton"

WarriorSkeleton::WarriorSkeleton(Position position, Rng& rng, const ItemCatalog& catalog,
                                 uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
