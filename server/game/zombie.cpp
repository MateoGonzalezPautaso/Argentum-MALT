#include "zombie.h"

#define MAX_HP_ZOMBIE 450
#define DAMAGE_ZOMBIE 15

Zombie::Zombie(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_ZOMBIE, DAMAGE_ZOMBIE, rng, equipable_items) {}
