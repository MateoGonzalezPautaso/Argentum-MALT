#include "iron_golem.h"

#define MAX_HP 1350
#define DAMAGE 55
#define NAME "Iron golem"

IronGolem::IronGolem(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
