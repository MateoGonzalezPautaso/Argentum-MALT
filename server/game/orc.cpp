#include "orc.h"

#define MAX_HP 600
#define DAMAGE 28
#define NAME "Orc"

Orc::Orc(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
