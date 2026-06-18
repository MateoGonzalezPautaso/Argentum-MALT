#include "entity.h"

Entity::Entity(uint32_t hp_max, const std::string& name, Position pos, uint8_t level,
               uint16_t sprite_id):
        hp_current(hp_max), name(name), pos(pos), level(level), hp_max(hp_max), sprite_id(sprite_id) {
}

bool Entity::try_attack(uint32_t current_tick, uint32_t cooldown_ticks) {
    if (current_tick < next_attack_tick)
        return false;
    next_attack_tick = current_tick + cooldown_ticks;
    return true;
}

bool Entity::is_dead() const { return hp_current == 0; }

void Entity::take_damage(uint32_t damage_taken) {
    if (damage_taken <= hp_current)
        hp_current -= damage_taken;
    else
        hp_current = 0;
}
