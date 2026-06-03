#ifndef BIG_SPIDER_H
#define BIG_SPIDER_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class BigSpider: public EnemyNpc {
public:
    BigSpider(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level);
};

#endif
