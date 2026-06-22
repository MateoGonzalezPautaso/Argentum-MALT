#ifndef BANK_SERVICE_H
#define BANK_SERVICE_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "../../../common/item_catalog.h"
#include "../../../common/messages.h"
#include "../../core/config.h"
#include "../command_result.h"
#include "../map.h"
#include "../player.h"

class BankService {
public:
    BankService(std::map<uint16_t, Player>& players,
                const std::unordered_map<std::string, Map>& maps, const ItemCatalog& item_catalog,
                const BalanceConfig& balance, const MessagesConfig& msgs);

    CommandResult handle_bank_deposit(uint16_t player_id, const BankDepositCmd& cmd);
    CommandResult handle_bank_withdraw(uint16_t player_id, const BankWithdrawCmd& cmd);
    BankUpdateEvent make_bank_update_event(const Player& p) const;

private:
    std::map<uint16_t, Player>& players_;
    const std::unordered_map<std::string, Map>& maps_;
    const ItemCatalog& item_catalog_;
    const BalanceConfig& balance_;
    const MessagesConfig& msgs_;
};

#endif  // BANK_SERVICE_H
