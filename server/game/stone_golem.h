#ifndef STONE_GOLEM_H
#define STONE_GOLEM_H

#include "../../common/item_catalog.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Dungeon boss
class StoneGolem: public EnemyNpc {
public:
    StoneGolem(Position position, Rng& rng, ItemCatalog& catalog, uint32_t level);
};

#endif
