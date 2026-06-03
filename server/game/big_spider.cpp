#include "big_spider.h"

#define MAX_HP 325
#define DAMAGE 17
#define NAME "Big spider"

BigSpider::BigSpider(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
