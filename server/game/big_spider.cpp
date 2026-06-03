#include "big_spider.h"

#define MAX_HP_BIG_SPIDER 325
#define DAMAGE_BIG_SPIDER 17

BigSpider::BigSpider(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP_BIG_SPIDER * level, DAMAGE_BIG_SPIDER * level, rng,
                 catalog, level) {}
