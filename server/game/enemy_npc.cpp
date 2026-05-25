#include "enemy_npc.h"

#include "../common/item.h"

EnemyNpc::EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
                   EquipableItems& equipable_items):
        rng(rng), equipable_items(equipable_items) {
    this->position = position;
    hp_current = hp_max;
    this->hp_max = hp_max;
    this->damage = damage;
}

void EnemyNpc::take_damage(uint32_t damage_taken) {
    if (damage_taken <= hp_current)
        hp_current -= damage_taken;
    else
        hp_current = 0;
}

EnemyDrop EnemyNpc::get_kill_reward() {
    double random_number = rng.get_random_double(0, 99);
    EnemyDrop enemy_drop{.gold = 0, .is_potion = false, .is_object = false};

    if (random_number < 8) {
        enemy_drop.gold = get_gold_reward();
    } else if ((random_number >= 8) && (random_number < 9)) {
        enemy_drop.is_potion = true;
        enemy_drop.item = get_potion();
    } else if ((random_number >= 9) && (random_number < 10)) {
        enemy_drop.is_object = true;
        enemy_drop.item = equipable_items.get_random_item();
    }
    return enemy_drop;
}

bool EnemyNpc::is_dead() { return hp_current == 0; }

uint32_t EnemyNpc::get_damage() const { return damage; }

uint32_t EnemyNpc::get_gold_reward() {
    double random_number = rng.get_random_double(0.01, 0.2);
    return random_number * hp_max;
}

Item EnemyNpc::get_potion() {
    int random_number = rng.get_random_int(1, 2);

    if (random_number == 1)
        return Item("Mana potion", ItemType::MANA_POTION, 0, 0, 0, 0, 0);
    else
        return Item("Health potion", ItemType::HEALTH_POTION, 0, 0, 0, 0, 0);
}
