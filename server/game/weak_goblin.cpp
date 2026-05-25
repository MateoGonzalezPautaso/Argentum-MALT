#include "weak_goblin.h"

#define MAX_HP_WEAK_GOBLIN 120
#define DAMAGE_WEAK_GOBLIN 5

WeakGoblin::WeakGoblin(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_WEAK_GOBLIN, DAMAGE_WEAK_GOBLIN, rng, equipable_items) {}
