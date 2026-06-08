#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>
#include <string>

#include "../../common/messages.h"

class Entity {
private:
    uint32_t next_attack_tick = 0;
    uint32_t hp_current;
    std::string name;
    Position pos;
    uint8_t level;
    uint32_t hp_max;
    std::string current_map = "main";

public:
    Entity(uint32_t hp_max, const std::string& name, Position pos, uint8_t level);
    bool try_attack(uint32_t current_tick, uint32_t cooldown_ticks);
    uint32_t get_hp_current() const { return hp_current; }
    void kill() { hp_current = 0; }
    const std::string& get_name() const { return name; }
    Position get_pos() const { return pos; }
    uint16_t pos_x() const { return pos.x; }
    uint16_t pos_y() const { return pos.y; }
    void set_pos(uint16_t x, uint16_t y) { pos = {x, y}; }
    uint8_t get_level() const { return level; }
    uint32_t get_hp_max() const { return hp_max; }
    bool is_dead() const;
    void set_hp_current(uint32_t new_hp_current) { hp_current = new_hp_current; }
    void set_hp_max(uint32_t new_hp_max) { hp_max = new_hp_max; }
    void set_level(uint8_t new_level) { level = new_level; }
    const std::string& get_current_map() const { return current_map; }
    void set_current_map(const std::string& map) { current_map = map; }
    virtual void take_damage(uint32_t damage);
};

#endif
