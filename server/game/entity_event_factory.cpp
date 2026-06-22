#include "entity_event_factory.h"

namespace EntityEventFactory {

EntitySpawnEvent make_entity_spawn(const Player& p) {
    const ItemType weapon_type = p.get_equipped(EquipSlot::WEAPON).item_type;
    const ItemType armor_type = p.get_equipped(EquipSlot::ARMOR).item_type;
    const ItemType helmet_type = p.get_equipped(EquipSlot::HELMET).item_type;
    const ItemType shield_type = p.get_equipped(EquipSlot::SHIELD).item_type;
    return EntitySpawnEvent{
            .entity_id = p.get_id(),
            .entity_type = EntityType::PLAYER,
            .entity_pos = p.get_pos(),
            .entity_dir = p.get_dir(),
            .entity_name = p.get_name(),
            .entity_race = p.get_race(),
            .entity_class = p.get_player_class(),
            .weapon_type = weapon_type,
            .armor_type = armor_type,
            .helmet_type = helmet_type,
            .shield_type = shield_type,
            .clan_name = p.get_clan_name(),
    };
}

EntitySpawnEvent make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) {
    return EntitySpawnEvent{
            .entity_id = npc_id,
            .entity_type = EntityType::NPC,
            .entity_pos = npc.get_pos(),
            .entity_dir = Direction::SOUTH,
            .entity_name = npc.get_name(),
            .entity_race = Race::HUMAN,
            .entity_class = PlayerClass::WARRIOR,
            .sprite_id = npc.get_sprite_id(),
            .clan_name = "",
    };
}

EquipUpdateEvent make_equip_update(uint16_t player_id, const Player& p) {
    InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
    p.dump_equipped(equipped_slots);
    return EquipUpdateEvent{player_id, equipped_slots[0], equipped_slots[1], equipped_slots[2],
                             equipped_slots[3]};
}

}  // namespace EntityEventFactory
