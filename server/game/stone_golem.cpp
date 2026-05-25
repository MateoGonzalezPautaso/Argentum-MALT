#include "stone_golem.h"

#define MAX_HP_STONE_GOLEM 1000
#define DAMAGE_STONE_GOLEM 40

StoneGolem::StoneGolem(Position position, Rng& rng, EquipableItems& equipable_items):
        EnemyNpc(position, MAX_HP_STONE_GOLEM, DAMAGE_STONE_GOLEM, rng, equipable_items) {}
