#ifndef SMALL_SPIDER_H
#define SMALL_SPIDER_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Very weak, enemy for newbies
class SmallSpider: public EnemyNpc {
public:
    SmallSpider(Position position, Rng& rng, EquipableItems& equipable_items);
};

#endif
