#ifndef STONE_GOLEM_H
#define STONE_GOLEM_H

#include "../../common/equipable_items.h"
#include "../../common/rng.h"

#include "enemy_npc.h"

// Dungeon boss
class StoneGolem: public EnemyNpc {
public:
    StoneGolem(Position position, Rng& rng, EquipableItems& equipable_items, uint32_t level);
};

#endif
