#include "chat_command_parser.h"

#include <string_view>

namespace {
template<typename Cmd>
ClientCommand wrap(std::string arg) { return Cmd{std::move(arg)}; }
}  // namespace

ChatCommandParser::ChatCommandParser() {
    exact_commands_ = {
        {"/listar",       NpcListCmd{}},
        {"/meditar",      MeditateCmd{}},
        {"/resucitar",    ResurrectCmd{}},
        {"/curar",        NpcHealCmd{}},
        {"/tomar",        PickupItemCmd{""}},
        {"/revisar-clan", ClanReviewCmd{}},
        {"/dejar-clan",   ClanLeaveCmd{}},
    };

    prefix_commands_ = {
        {"/clan-rechazar ", wrap<ClanRejectCmd>},
        {"/clan-aceptar ",  wrap<ClanAcceptCmd>},
        {"/clan-unban ",    wrap<ClanUnbanCmd>},
        {"/clan-kick ",     wrap<ClanKickCmd>},
        {"/clan-ban ",      wrap<ClanBanCmd>},
        {"/fundar-clan ",   wrap<ClanFoundCmd>},
        {"/comprar ",       wrap<NpcBuyCmd>},
        {"/vender ",        wrap<NpcSellCmd>},
        {"/unirse ",        wrap<ClanJoinRequestCmd>},
        {"/tomar ",         wrap<PickupItemCmd>},
        {"/tirar ",         wrap<DropItemCmd>},
    };
}

std::optional<ClientCommand> ChatCommandParser::parse(const std::string& text) const {
    auto arg = [&](std::string_view prefix) -> std::string {
        return text.substr(prefix.size());
    };

    if (auto it = exact_commands_.find(text); it != exact_commands_.end())
        return it->second;

    // Casos especiales con parsing numérico o lógica propia.
    if (text.starts_with("/depositar oro ")) {
        try { return BankDepositCmd{true, "", static_cast<uint32_t>(std::stoul(arg("/depositar oro ")))}; }
        catch (...) { return std::nullopt; }
    }
    if (text.starts_with("/retirar oro ")) {
        try { return BankWithdrawCmd{true, "", static_cast<uint32_t>(std::stoul(arg("/retirar oro ")))}; }
        catch (...) { return std::nullopt; }
    }
    if (text.starts_with("/depositar ")) return BankDepositCmd{false, arg("/depositar "), 0};
    if (text.starts_with("/retirar "))   return BankWithdrawCmd{false, arg("/retirar "), 0};
    if (text.starts_with("/equipar ")) {
        try { return EquipItemCmd{static_cast<uint8_t>(std::stoi(arg("/equipar ")))}; }
        catch (...) { return std::nullopt; }
    }
    if (text.starts_with("/desequipar ")) {
        const std::string slot = arg("/desequipar ");
        if (slot == "weapon") return UnequipItemCmd{EquipSlot::WEAPON};
        if (slot == "armor")  return UnequipItemCmd{EquipSlot::ARMOR};
        if (slot == "helmet") return UnequipItemCmd{EquipSlot::HELMET};
        if (slot == "shield") return UnequipItemCmd{EquipSlot::SHIELD};
        return std::nullopt;
    }

    // Comandos simples: prefijo + string directo → Cmd{arg}
    for (const auto& [prefix, fn] : prefix_commands_) {
        if (text.starts_with(prefix))
            return fn(arg(prefix));
    }

    return std::nullopt;
}
