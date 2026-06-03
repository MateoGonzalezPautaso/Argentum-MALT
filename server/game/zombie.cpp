#include "zombie.h"

#define MAX 375
#define DAMAGE 15
#define NAME "Zombie"

Zombie::Zombie(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level):
        EnemyNpc(position, MAX * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
