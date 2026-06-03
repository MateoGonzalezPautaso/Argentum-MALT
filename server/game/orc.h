#ifndef ORC_H
#define ORC_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

class Orc: public EnemyNpc {
public:
    Orc(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level);
};

#endif
