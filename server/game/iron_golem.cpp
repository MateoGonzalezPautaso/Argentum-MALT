#include "iron_golem.h"

#define MAX_HP_IRON_GOLEM 1350
#define DAMAGE_IRON_GOLEM 55

IronGolem::IronGolem(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP_IRON_GOLEM * level, DAMAGE_IRON_GOLEM * level, rng,
                 catalog, level) {}
