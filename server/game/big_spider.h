#ifndef BIG_SPIDER_H
#define BIG_SPIDER_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// More threatening that SmallSpider
class BigSpider: public EnemyNpc {
public:
    BigSpider(Position position, Rng& rng, EquipableItems& equipable_items, uint32_t level);
};

#endif
