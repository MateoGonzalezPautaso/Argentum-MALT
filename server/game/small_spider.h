#ifndef SMALL_SPIDER_H
#define SMALL_SPIDER_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class SmallSpider: public EnemyNpc {
public:
    SmallSpider(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
