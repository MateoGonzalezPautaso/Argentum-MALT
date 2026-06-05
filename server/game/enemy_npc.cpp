#include "enemy_npc.h"

EnemyNpc::EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
                   const ItemCatalog& catalog, uint32_t level, const std::string& name):
        position(position),
        hp_current(hp_max),
        hp_max(hp_max),
        damage(damage),
        rng(rng),
        catalog(catalog),
        level(level),
        name(name) {}

void EnemyNpc::take_damage(uint32_t damage_taken) {
    if (damage_taken <= hp_current)
        hp_current -= damage_taken;
    else
        hp_current = 0;
}

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

bool EnemyNpc::is_dead() const { return hp_current == 0; }

uint32_t EnemyNpc::get_damage() const { return damage; }

uint32_t EnemyNpc::get_gold_reward() {
    double random_number = rng.get_random_double(0.0, 0.2);
    return random_number * hp_max;
}

uint32_t EnemyNpc::get_level() const { return level; }

uint32_t EnemyNpc::get_hp_max() const { return hp_max; }

uint32_t EnemyNpc::get_hp_current() const { return hp_current; }

uint16_t EnemyNpc::pos_x() const { return position.x; }

uint16_t EnemyNpc::pos_y() const { return position.y; }

std::string EnemyNpc::get_name() const { return name; }
