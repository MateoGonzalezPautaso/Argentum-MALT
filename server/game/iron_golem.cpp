#include "iron_golem.h"

#define MAX_HP_IRON_GOLEM 1500
#define DAMAGE_IRON_GOLEM 55

IronGolem::IronGolem(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_IRON_GOLEM, DAMAGE_IRON_GOLEM, rng, equipable_items) {}
