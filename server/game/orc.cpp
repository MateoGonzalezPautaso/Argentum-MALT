#include "orc.h"

#define MAX_HP 600
#define DAMAGE 28
#define NAME "Orc"

Orc::Orc(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
