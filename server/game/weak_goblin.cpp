#include "weak_goblin.h"

#define MAX_HP 100
#define DAMAGE 5
#define NAME "Weak goblin"

WeakGoblin::WeakGoblin(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
