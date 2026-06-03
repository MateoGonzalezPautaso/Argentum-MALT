#ifndef ORC_H
#define ORC_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class Orc: public EnemyNpc {
public:
    Orc(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level);
};

#endif
