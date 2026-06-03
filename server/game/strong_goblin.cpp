#include "strong_goblin.h"

#define MAX_HP 250
#define DAMAGE 12
#define NAME "Strong goblin"

StrongGoblin::StrongGoblin(Position position, Rng& rng, EquipableItems& equipable_items,
                           uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
