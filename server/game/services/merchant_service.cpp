#include "merchant_service.h"

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

bool vendor_sells(const VendorsConfig& vendors, const std::string& vendor, ItemType type) {
    auto it = vendors.by_vendor.find(vendor);
    return it != vendors.by_vendor.end() && it->second.count(type) > 0;
}

}  // namespace

MerchantService::MerchantService(std::map<uint16_t, Player>& players,
                                 std::unordered_map<std::string, Map>& maps,
                                 const ItemCatalog& item_catalog, const BalanceConfig& balance,
                                 BankService& bank_service):
        players_(players),
        maps_(maps),
        item_catalog_(item_catalog),
        balance_(balance),
        bank_service_(bank_service) {}

std::variant<MerchantService::VendorContext, CommandResult> MerchantService::resolve_vendor_ctx(
        uint16_t player_id, const std::string& item_name, const std::string& action) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return CommandResult{};

    Player& player = it->second;

    if (player.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los fantasmas no pueden " + action};
        return CommandResult{.private_events = {msg}};
    }

    if (item_name.empty()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Uso: /" + action + " <nombre del objeto>"};
        return CommandResult{.private_events = {msg}};
    }

    auto map_it = maps_.find(player.get_current_map());
    if (map_it == maps_.end())
        return CommandResult{};

    Map& map = map_it->second;
    return VendorContext{.player = &player,
                         .map = &map,
                         .px = static_cast<int>(player.pos_x()),
                         .py = static_cast<int>(player.pos_y()),
                         .range = map.tile_size() * balance_.merchant.interaction_range_tiles};
}

CommandResult MerchantService::handle_npc_list(uint16_t player_id) {
    auto it = players_.find(player_id);
    if (it == players_.end())
        return {};

    const Player& player = it->second;

    if (player.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los fantasmas no pueden listar"};
        return {.private_events = {msg}};
    }

    auto map_it = maps_.find(player.get_current_map());
    if (map_it == maps_.end())
        return {};

    const Map& map = map_it->second;
    const int range = map.tile_size() * balance_.merchant.interaction_range_tiles;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    const bool near_comerciante = map.prop_grid().is_in_range_of("comerciante", px, py, range);
    const bool near_sacerdote = map.prop_grid().is_in_range_of("sacerdote", px, py, range);
    const bool near_banquero = map.prop_grid().is_in_range_of("banquero", px, py, range);

    if (near_banquero && !near_comerciante && !near_sacerdote) {
        return {.private_events = {bank_service_.make_bank_update_event(player)}};
    }

    if (!near_comerciante && !near_sacerdote) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No hay un comerciante, sacerdote ni banquero cerca"};
        return {.private_events = {msg}};
    }

    const std::string vendor_name = near_comerciante ? "comerciante" : "sacerdote";

    std::vector<NpcItemEntry> items;
    for (const auto& item: item_catalog_.all()) {
        if (vendor_sells(balance_.vendors, vendor_name, item.type))
            items.push_back({item.name, item.type, 0, item.price});
    }

    return {.private_events = {NpcItemListEvent{std::move(items)}}};
}

CommandResult MerchantService::handle_npc_buy(uint16_t player_id, const NpcBuyCmd& cmd) {
    const std::string& item_name = cmd.item_name;
    auto ctx_or_err = resolve_vendor_ctx(player_id, item_name, "comprar");
    if (auto* err = std::get_if<CommandResult>(&ctx_or_err))
        return std::move(*err);
    VendorContext ctx = std::get<VendorContext>(ctx_or_err);
    Player& player = *ctx.player;

    const bool near_sacerdote =
            ctx.map->prop_grid().is_in_range_of("sacerdote", ctx.px, ctx.py, ctx.range);
    const bool near_comerciante =
            ctx.map->prop_grid().is_in_range_of("comerciante", ctx.px, ctx.py, ctx.range);

    if (!near_sacerdote && !near_comerciante) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No hay un sacerdote ni un comerciante cerca"};
        return {.private_events = {msg}};
    }

    const Item* found = item_catalog_.find_by_name(item_name);

    if (!found) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Objeto '" + item_name + "' no encontrado"};
        return {.private_events = {msg}};
    }

    const VendorsConfig& vendors = balance_.vendors;
    const bool sacerdote_sells = near_sacerdote && vendor_sells(vendors, "sacerdote", found->type);
    const bool comerciante_sells =
            near_comerciante && vendor_sells(vendors, "comerciante", found->type);

    if (found->price == 0 || (!sacerdote_sells && !comerciante_sells)) {
        std::string vendor_label;
        if (near_sacerdote && near_comerciante) {
            vendor_label = "Ningún NPC cercano";
        } else if (near_comerciante) {
            vendor_label = "El comerciante";
        } else {
            vendor_label = "El sacerdote";
        }
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", vendor_label + " no vende ese objeto"};
        return {.private_events = {msg}};
    }

    if (player.get_gold() < found->price) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "Oro insuficiente. El " + found->name + " cuesta " +
                                 std::to_string(found->price) + " de oro"};
        return {.private_events = {msg}};
    }

    player.spend_gold(found->price);

    if (!player.add_item(found->type, found->name)) {
        player.gain_gold(found->price);
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Inventario lleno"};
        return {.private_events = {msg}};
    }

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    GoldUpdateEvent gold_ev{player.get_gold()};
    ChatMsgEvent msg{
            ChatMsgType::SYSTEM, "",
            "Compraste " + found->name + " por " + std::to_string(found->price) + " de oro"};
    return {.private_events = {msg, inv_ev, gold_ev}};
}

CommandResult MerchantService::handle_npc_sell(uint16_t player_id, const NpcSellCmd& cmd) {
    const std::string& item_name = cmd.item_name;
    auto ctx_or_err = resolve_vendor_ctx(player_id, item_name, "vender");
    if (auto* err = std::get_if<CommandResult>(&ctx_or_err))
        return std::move(*err);
    VendorContext ctx = std::get<VendorContext>(ctx_or_err);
    Player& player = *ctx.player;

    if (!ctx.map->prop_grid().is_in_range_of("comerciante", ctx.px, ctx.py, ctx.range)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No hay un comerciante cerca"};
        return {.private_events = {msg}};
    }

    const Item* item_def = item_catalog_.find_by_name(item_name);
    if (!item_def) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Objeto '" + item_name + "' no encontrado"};
        return {.private_events = {msg}};
    }

    if (!vendor_sells(balance_.vendors, "comerciante", item_def->type)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "El comerciante no compra ese tipo de objeto"};
        return {.private_events = {msg}};
    }

    auto slot_opt = player.find_slot_by_type(item_def->type);
    if (!slot_opt) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No tenés '" + item_def->name + "' en el inventario"};
        return {.private_events = {msg}};
    }

    uint32_t sell_price =
            static_cast<uint32_t>(item_def->price * balance_.merchant.sell_price_ratio);

    player.remove_inventory_item(static_cast<uint8_t>(slot_opt->slot_index));
    player.gain_gold(sell_price);

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    GoldUpdateEvent gold_ev{player.get_gold()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "Vendiste " + item_name + " por " + std::to_string(sell_price) + " de oro"};
    return {.private_events = {msg, inv_ev, gold_ev}};
}
