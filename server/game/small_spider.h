#ifndef SMALL_SPIDER_H
#define SMALL_SPIDER_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class SmallSpider: public EnemyNpc {
public:
    SmallSpider(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level);
};

#endif
