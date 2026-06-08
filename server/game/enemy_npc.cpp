#include "enemy_npc.h"

EnemyNpc::EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
                   const ItemCatalog& catalog, uint8_t level, const std::string& name):
        damage(damage), rng(rng), catalog(catalog), Entity(hp_max, name, position, level) {}

EnemyDrop EnemyNpc::get_kill_reward() {
    double random_number = rng.get_random_double(0, 99);
    EnemyDrop enemy_drop{.gold = 0, .is_potion = false, .is_object = false, .item = std::nullopt};

    if (random_number < 8) {
        enemy_drop.gold = get_gold_reward();
    } else if ((random_number >= 8) && (random_number < 9)) {
        enemy_drop.is_potion = true;
        const Item* potion = catalog.find(rng.get_random_int(1, 2) == 1 ? ItemType::MANA_POTION :
                                                                          ItemType::HEALTH_POTION);
        if (potion)
            enemy_drop.item = *potion;
    } else if ((random_number >= 9) && (random_number < 10)) {
        enemy_drop.is_object = true;
        enemy_drop.item = catalog.random_equipable(rng);
    }
    return enemy_drop;
}

uint32_t EnemyNpc::get_damage() const { return damage; }

uint32_t EnemyNpc::get_gold_reward() {
    double random_number = rng.get_random_double(0.01, 0.2);
    return random_number * get_hp_max();
}
