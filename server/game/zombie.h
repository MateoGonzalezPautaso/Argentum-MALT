#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "../common/equipable_items.h"
#include "../common/rng.h"

#include "enemy_npc.h"

class Zombie: public EnemyNpc {
public:
    Zombie(Position position, Rng& rng, EquipableItems& equipable_items);
};

#endif
