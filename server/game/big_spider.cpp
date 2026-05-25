#include "big_spider.h"

#define MAX_HP_BIG_SPIDER 400
#define DAMAGE_BIG_SPIDER 17

BigSpider::BigSpider(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_BIG_SPIDER, DAMAGE_BIG_SPIDER, rng, equipable_items) {}
