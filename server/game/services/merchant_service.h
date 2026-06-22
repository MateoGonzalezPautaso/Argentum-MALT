#ifndef MERCHANT_SERVICE_H
#define MERCHANT_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>

#include "../../../common/item_catalog.h"
#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../map.h"
#include "../player.h"

#include "bank_service.h"

class MerchantService {
public:
    MerchantService(std::map<uint16_t, Player>& players, std::unordered_map<std::string, Map>& maps,
                    const ItemCatalog& item_catalog, const BalanceConfig& balance,
                    BankService& bank_service, const MessagesConfig& msgs);

    CommandResult handle_npc_buy(uint16_t player_id, const NpcBuyCmd& cmd);
    CommandResult handle_npc_sell(uint16_t player_id, const NpcSellCmd& cmd);
    CommandResult handle_npc_list(uint16_t player_id);

private:
    struct VendorContext {
        Player* player;
        Map* map;
        int px;
        int py;
        int range;
    };

    std::variant<VendorContext, CommandResult> resolve_vendor_ctx(uint16_t player_id,
                                                                  const std::string& item_name,
                                                                  const std::string& action);

    std::map<uint16_t, Player>& players_;
    std::unordered_map<std::string, Map>& maps_;
    const ItemCatalog& item_catalog_;
    const BalanceConfig& balance_;
    BankService& bank_service_;
    const MessagesConfig& msgs_;
};

#endif  // MERCHANT_SERVICE_H
