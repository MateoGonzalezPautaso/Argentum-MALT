#include "strong_goblin.h"

#define MAX_HP 250
#define DAMAGE 12
#define NAME "Strong goblin"

StrongGoblin::StrongGoblin(Position position, Rng& rng, const ItemCatalog& catalog,
                           uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
