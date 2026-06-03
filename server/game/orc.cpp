#include "orc.h"

#define MAX_HP_ORC 600
#define DAMAGE_ORC 28

Orc::Orc(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP_ORC * level, DAMAGE_ORC * level, rng, catalog, level) {}
