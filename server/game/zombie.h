#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class Zombie: public EnemyNpc {
public:
    Zombie(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
