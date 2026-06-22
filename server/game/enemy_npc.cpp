#include "enemy_npc.h"

#include "game_formulas.h"

EnemyNpc::EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
                   const ItemCatalog& catalog, uint8_t level, const std::string& name,
                   uint16_t sprite_id, uint32_t speed):
        Entity(hp_max, name, position, level, sprite_id, speed),
        damage(damage),
        rng(rng),
        catalog(catalog) {}

EnemyDrop EnemyNpc::get_kill_reward(const NpcDropConfig& drop_config,
                                    const BalanceConfig& balance) {
    double r = rng.get_random_double(0, 100);
    EnemyDrop enemy_drop{.gold = 0, .is_potion = false, .is_object = false, .item = std::nullopt};

    if (r < drop_config.gold_chance) {
        enemy_drop.gold = GameFormulas::npc_gold_reward(balance, get_hp_max(), rng);
    } else if (r < drop_config.gold_chance + drop_config.potion_chance) {
        enemy_drop.is_potion = true;
        const Item* potion = catalog.find(rng.get_random_int(1, 2) == 1 ? ItemType::MANA_POTION :
                                                                          ItemType::HEALTH_POTION);
        if (potion)
            enemy_drop.item = *potion;
    } else if (r < drop_config.gold_chance + drop_config.potion_chance + drop_config.item_chance) {
        enemy_drop.is_object = true;
        enemy_drop.item = catalog.random_equipable(rng);
    }
    return enemy_drop;
}

uint32_t EnemyNpc::get_damage() const { return damage; }
