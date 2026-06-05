#include "stone_golem.h"

#define MAX_HP 900
#define DAMAGE 40
#define NAME "Stone golem"

StoneGolem::StoneGolem(Position position, Rng& rng, const ItemCatalog& catalog, uint32_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, catalog, level, NAME) {}
