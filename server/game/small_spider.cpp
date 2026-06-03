#include "small_spider.h"

#define MAX_HP 80
#define DAMAGE 4
#define NAME "Small spider"

SmallSpider::SmallSpider(Position position, Rng& rng, EquipableItems& equipable_items,
                         uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
