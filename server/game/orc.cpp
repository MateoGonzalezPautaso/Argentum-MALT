#include "orc.h"

#define MAX_HP_ORC 700
#define DAMAGE_ORC 28

Orc::Orc(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_ORC, DAMAGE_ORC, rng, equipable_items) {}
