#ifndef ENEMY_NPC_H
#define ENEMY_NPC_H

#include <optional>
#include <string>

#include "../../common/item_catalog.h"
#include "../../common/rng.h"
#include "../core/config.h"

#include "entity.h"

struct EnemyDrop {
    uint32_t gold;
    bool is_potion;
    bool is_object;
    std::optional<Item> item;
};

class EnemyNpc: public Entity {
private:
    uint32_t damage;
    Rng& rng;
    const ItemCatalog& catalog;
    uint32_t idle_move_timer = 0;
    Direction idle_move_dir = Direction::SOUTH;

public:
    EnemyNpc(Position position, uint32_t hp_max, uint32_t damage, Rng& rng,
             const ItemCatalog& catalog, uint8_t level, const std::string& name,
             uint16_t sprite_id = 0, uint32_t speed = 2);
    EnemyDrop get_kill_reward(const NpcDropConfig& drop_config, const BalanceConfig& balance);
    uint32_t get_damage() const;
    void set_idle_move_timer(uint32_t new_timer) { idle_move_timer = new_timer; }
    uint32_t get_idle_move_timer() const { return idle_move_timer; }
    void set_idle_move_dir(Direction new_dir) { idle_move_dir = new_dir; }
    Direction get_idle_move_dir() { return idle_move_dir; }
};

#endif
