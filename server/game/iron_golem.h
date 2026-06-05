#ifndef IRON_GOLEM_H
#define IRON_GOLEM_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class IronGolem: public EnemyNpc {
public:
    IronGolem(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
