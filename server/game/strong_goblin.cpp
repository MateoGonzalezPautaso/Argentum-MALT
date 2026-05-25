#include "strong_goblin.h"

#define MAX_HP_STRONG_GOB 300
#define DAMAGE_STRONG_GOBLIN 12

StrongGoblin::StrongGoblin(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_STRONG_GOB, DAMAGE_STRONG_GOBLIN, rng, equipable_items) {}
