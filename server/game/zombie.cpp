#include "zombie.h"

#define MAX_HP_ZOMBIE 375
#define DAMAGE_ZOMBIE 15

Zombie::Zombie(Position position, Rng& rng, EquipableItems& equipable_items, uint32_t level):
        EnemyNpc(position, MAX_HP_ZOMBIE * level, DAMAGE_ZOMBIE * level, rng, equipable_items,
                 level) {}
