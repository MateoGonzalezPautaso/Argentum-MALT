#include "strong_goblin.h"

#define MAX_HP_STRONG_GOB 250
#define DAMAGE_STRONG_GOBLIN 12

StrongGoblin::StrongGoblin(Position position, Rng& rng, ItemCatalog& catalog,
                           uint32_t level):
        EnemyNpc(position, MAX_HP_STRONG_GOB * level, DAMAGE_STRONG_GOBLIN * level, rng,
                 catalog, level) {}
