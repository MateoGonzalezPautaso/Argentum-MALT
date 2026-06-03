#include "zombie.h"

#define MAX_HP 375
#define DAMAGE 15
#define NAME "Zombie"

Zombie::Zombie(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
