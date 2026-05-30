#include "stone_golem.h"

#define MAX_HP_STONE_GOLEM 900
#define DAMAGE_STONE_GOLEM 40

StoneGolem::StoneGolem(Position position, Rng& rng, EquipableItems& equipable_items,
                       uint32_t level):
        EnemyNpc(position, MAX_HP_STONE_GOLEM * level, DAMAGE_STONE_GOLEM * level, rng,
                 equipable_items, level) {}
