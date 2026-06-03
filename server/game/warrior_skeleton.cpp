#include "warrior_skeleton.h"

#define MAX_HP 425
#define DAMAGE 22
#define NAME "Warrior skeleton"

WarriorSkeleton::WarriorSkeleton(Position position, Rng& rng, EquipableItems& equipable_items,
                                 uint8_t level):
        EnemyNpc(position, MAX_HP * level, DAMAGE * level, rng, equipable_items, level, NAME) {}
