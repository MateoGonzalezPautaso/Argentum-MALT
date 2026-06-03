#include "small_spider.h"

#define MAX_HP_SMALL_SPIDER 80
#define DAMAGE_SMALL_SPIDER 4

SmallSpider::SmallSpider(Position position, Rng& rng, ItemCatalog& catalog,
                         uint32_t level):
        EnemyNpc(position, MAX_HP_SMALL_SPIDER * level, DAMAGE_SMALL_SPIDER * level, rng,
                 catalog, level) {}
