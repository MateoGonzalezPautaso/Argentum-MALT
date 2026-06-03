#ifndef BIG_SPIDER_H
#define BIG_SPIDER_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// More threatening that SmallSpider
class BigSpider: public EnemyNpc {
public:
    BigSpider(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level);
};

#endif
