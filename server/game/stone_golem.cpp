#include "stone_golem.h"

#define MAX_HP 900
#define DAMAGE 40
#define NAME "Stone golem"

StoneGolem::StoneGolem(Position position, Rng& rng, EquipableItems& equipable_items, uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
