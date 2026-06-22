#include "player_session_service.h"

#include <string>
#include <vector>

PlayerSessionService::PlayerSessionService(
        std::map<uint16_t, Player>& players,
        std::unordered_map<std::string, uint16_t>& player_name_index,
        PlayerDataService& player_data_service, std::unordered_map<std::string, Map>& maps,
        const std::map<uint16_t, EnemyNpc>& enemy_npcs, const BalanceConfig& balance,
        const InventoryConfig& inventory_config, const ItemCatalog& item_catalog,
        ClanManager& clan_manager, ClanCommandHandler& clan_handler,
        CombatController& combat_controller, GroundItemService& ground_item_service):
        players_(players),
        player_name_index_(player_name_index),
        player_data_service_(player_data_service),
        maps_(maps),
        enemy_npcs_(enemy_npcs),
        balance_(balance),
        inventory_config_(inventory_config),
        item_catalog_(item_catalog),
        clan_manager_(clan_manager),
        clan_handler_(clan_handler),
        combat_controller_(combat_controller),
        ground_item_service_(ground_item_service) {}

bool PlayerSessionService::is_username_logged_in(const std::string& username) const {
    return player_name_index_.count(username) > 0;
}

LoginOkEvent PlayerSessionService::make_login_ok(const Player& p) const {
    return LoginOkEvent{
            .player_id = p.get_id(),
            .username = p.get_name(),
            .race = p.get_race(),
            .player_class = p.get_player_class(),
            .level = p.get_level(),
            .experience = p.get_experience(),
            .exp_to_next = p.exp_to_next_level(),
            .hp_current = p.get_hp_current(),
            .hp_max = p.get_hp_max(),
            .mana_current = p.get_mana_current(),
            .mana_max = p.get_mana_max(),
            .gold = p.get_gold(),
            .pos = p.get_pos(),
    };
}

EntitySpawnEvent PlayerSessionService::make_entity_spawn(const Player& p) const {
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

EntitySpawnEvent PlayerSessionService::make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const {
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

PlayerStatsEvent PlayerSessionService::make_player_stats_event(const Player& p) const {
    PlayerStatsEvent ev{};
    combat_controller_.fill_player_stats_event(ev, p);
    return ev;
}

void PlayerSessionService::append_existing_entities(std::vector<ServerEvent>& events,
                                                    uint16_t exclude_id,
                                                    const std::string& map_name) const {
    for (const auto& [id, player]: players_) {
        if (id == exclude_id)
            continue;
        if (player.get_current_map() != map_name)
            continue;
        events.push_back(make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs_) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != map_name)
            continue;
        events.push_back(make_npc_spawn(npc, id));
    }
    std::vector<ServerEvent> items = ground_item_service_.make_existing_ground_items(map_name);
    events.insert(events.end(), items.begin(), items.end());
}

CommandResult PlayerSessionService::handle_login(uint16_t player_id, const LoginCmd& cmd) {
    PlayerRecord rec;

    if (player_data_service_.player_exists(cmd.username)) {
        rec = player_data_service_.load_record(cmd.username);

        if (!rec.check_password(cmd.password)) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid password"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        if (is_username_logged_in(cmd.username)) {
            LoginErrorEvent err{LoginError::ALREADY_LOGGED_IN, "This user is already logged in"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        auto player_opt = player_data_service_.load_player(player_id, cmd.username, rec);
        if (!player_opt.has_value()) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Failed to load player"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        // Set clan membership
        std::string clan_name = clan_manager_.get_clan_name(cmd.username);
        player_opt->set_clan_name(clan_name);

        auto it = players_.emplace(player_id, std::move(*player_opt)).first;
        player_name_index_[it->second.get_name()] = player_id;
        const Player& p = it->second;

        EntitySpawnEvent spawn = make_entity_spawn(p);
        std::vector<ServerEvent> private_events = {make_login_ok(p)};

        // Send inventory after login
        InventoryUpdateEvent inv_event{p.dump_inventory()};
        private_events.push_back(inv_event);

        // Send equipment state after login
        InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
        p.dump_equipped(equipped_slots);
        EquipUpdateEvent equip_ev{player_id, equipped_slots[0], equipped_slots[1],
                                  equipped_slots[2], equipped_slots[3]};
        private_events.push_back(equip_ev);

        private_events.push_back(make_player_stats_event(p));

        if (p.get_current_map() != balance_.starting_map) {
            private_events.push_back(MapTransitionEvent{
                    .map_name = p.get_current_map(),
                    .pos_x = p.pos_x(),
                    .pos_y = p.pos_y(),
            });
        }

        append_existing_entities(private_events, p.get_id(), p.get_current_map());

        // Notify clan members of login
        CommandResult login_result;
        login_result.private_events = std::move(private_events);
        login_result.map_events = {spawn};

        if (!clan_name.empty()) {
            ClanNotificationEvent notif{ClanNotifType::MEMBER_ONLINE, cmd.username, clan_name};
            auto clan_result = clan_handler_.notify_clan_members(clan_name, notif, player_id);
            login_result.targeted_events = std::move(clan_result.targeted_events);
        }

        return login_result;
    }

    LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid username or password"};
    return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult PlayerSessionService::handle_create_character(uint16_t player_id,
                                                            const CreateCharacterCmd& cmd) {
    if (cmd.username.empty()) {
        CharacterErrorEvent err{CharacterError::INVALID_USERNAME, "Username cannot be empty"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (player_data_service_.player_exists(cmd.username)) {
        CharacterErrorEvent err{CharacterError::USERNAME_TAKEN,
                                "User " + cmd.username + " already exists"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (is_username_logged_in(cmd.username)) {
        CharacterErrorEvent err{CharacterError::USERNAME_TAKEN,
                                "User " + cmd.username + " is already in game"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    PlayerRecord rec;
    rec.set_username(cmd.username);
    rec.set_password(cmd.password);
    rec.race = static_cast<uint8_t>(cmd.race);
    rec.player_class = static_cast<uint8_t>(cmd.player_class);
    rec.level = 1;
    rec.experience = 0;
    rec.pos_x = static_cast<uint16_t>(balance_.starting_pos_x);
    rec.pos_y = static_cast<uint16_t>(balance_.starting_pos_y);
    rec.dir = static_cast<uint8_t>(Direction::SOUTH);
    rec.gold = static_cast<uint32_t>(balance_.starting_gold);

    Player player(player_id, cmd.username, Position{rec.pos_x, rec.pos_y}, Direction::SOUTH,
                  cmd.race, cmd.player_class, balance_, inventory_config_.max_slots,
                  inventory_config_.max_hp_potions, inventory_config_.max_mana_potions,
                  inventory_config_.max_bank_slots);
    player.set_current_map(balance_.starting_map);
    rec.hp_current = player.get_hp_current();
    rec.hp_max = player.get_hp_max();
    rec.mana_current = player.get_mana_current();
    rec.mana_max = player.get_mana_max();

    const auto& items_by_class = balance_.starting_items.by_class;
    auto class_it = items_by_class.find(cmd.player_class);
    if (class_it != items_by_class.end()) {
        for (ItemType type: class_it->second) {
            const Item* def = item_catalog_.find(type);
            if (def) {
                player.add_item(type, def->name);
            }
        }
    }

    player_data_service_.save_new_player(cmd.username, rec);
    player_data_service_.save_player(player);

    auto it = players_.emplace(player_id, std::move(player)).first;
    player_name_index_[it->second.get_name()] = player_id;
    const Player& p = it->second;

    CharacterCreatedEvent created{make_login_ok(p)};
    EntitySpawnEvent spawn = make_entity_spawn(p);
    std::vector<ServerEvent> private_events = {created};

    InventoryUpdateEvent inv_event{p.dump_inventory()};
    private_events.push_back(inv_event);

    InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
    p.dump_equipped(equipped_slots);
    EquipUpdateEvent equip_ev{player_id, equipped_slots[0], equipped_slots[1], equipped_slots[2],
                              equipped_slots[3]};
    private_events.push_back(equip_ev);

    private_events.push_back(make_player_stats_event(p));

    if (p.get_current_map() != balance_.starting_map) {
        private_events.push_back(MapTransitionEvent{
                .map_name = p.get_current_map(),
                .pos_x = p.pos_x(),
                .pos_y = p.pos_y(),
        });
    }

    append_existing_entities(private_events, p.get_id(), p.get_current_map());

    CommandResult r;
    r.private_events = std::move(private_events);
    r.map_events = {spawn};
    return r;
}

CommandResult PlayerSessionService::remove_player(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};

    std::string map_name = it->second.get_current_map();
    std::string clan_name = it->second.get_clan_name();
    std::string username = it->second.get_name();
    player_data_service_.save_player(it->second);

    EntityDespawnEvent despawn{.entity_id = player_id};

    CommandResult result;
    for (const auto& [id, player]: players_) {
        if (id == player_id)
            continue;
        if (player.get_current_map() != map_name)
            continue;
        result.targeted_events[id].push_back(despawn);
    }

    player_name_index_.erase(username);
    players_.erase(it);

    // Notify clan members of logout
    if (!clan_name.empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_OFFLINE, username, clan_name};
        auto clan_result = clan_handler_.notify_clan_members(notif.clan_name, notif, player_id);
        for (auto& ev: clan_result.targeted_events)
            for (auto& se: ev.second) result.targeted_events[ev.first].push_back(std::move(se));
    }

    return result;
}
