#include "small_spider.h"

#define MAX_HP_SMALL_SPIDER 100
#define DAMAGE_SMALL_SPIDER 4

SmallSpider::SmallSpider(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_SMALL_SPIDER, DAMAGE_SMALL_SPIDER, rng, equipable_items) {}
