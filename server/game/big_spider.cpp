#include "big_spider.h"

#define MAX_HP 325
#define DAMAGE 17
#define NAME "Big spider"

BigSpider::BigSpider(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
