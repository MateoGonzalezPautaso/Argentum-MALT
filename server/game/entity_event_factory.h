#ifndef ENTITY_EVENT_FACTORY_H
#define ENTITY_EVENT_FACTORY_H

#include <cstdint>

#include "../../common/messages.h"
#include "enemy_npc.h"
#include "player.h"

namespace EntityEventFactory {

EntitySpawnEvent make_entity_spawn(const Player& p);
EntitySpawnEvent make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id);

}  // namespace EntityEventFactory

#endif  // ENTITY_EVENT_FACTORY_H
