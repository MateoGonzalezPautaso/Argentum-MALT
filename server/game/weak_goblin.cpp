#include "weak_goblin.h"

#define MAX_HP_WEAK_GOBLIN 100
#define DAMAGE_WEAK_GOBLIN 5

WeakGoblin::WeakGoblin(Position position, Rng& rng, EquipableItems& equipable_items,
                       uint32_t level):
        EnemyNpc(position, MAX_HP_WEAK_GOBLIN * level, DAMAGE_WEAK_GOBLIN * level, rng,
                 equipable_items, level) {}
