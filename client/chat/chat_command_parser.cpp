#include "chat_command_parser.h"

#include <string_view>

std::optional<ClientCommand> ChatCommandParser::parse(const std::string& text) const {
    auto arg = [&](std::string_view prefix) -> std::string {
        return text.substr(prefix.size());
    };

    // Comandos exactos
    if (text == "/listar")       return NpcListCmd{};
    if (text == "/meditar")      return MeditateCmd{};
    if (text == "/resucitar")    return ResurrectCmd{};
    if (text == "/curar")        return NpcHealCmd{};
    if (text == "/tomar")        return PickupItemCmd{""};
    if (text == "/revisar-clan") return ClanReviewCmd{};
    if (text == "/dejar-clan")   return ClanLeaveCmd{};

    // Prefijos más largos primero para evitar match prematuro
    if (text.starts_with("/depositar oro ")) {
        try { return BankDepositCmd{true, "", static_cast<uint32_t>(std::stoul(arg("/depositar oro ")))}; }
        catch (...) { return std::nullopt; }
    }
    if (text.starts_with("/retirar oro ")) {
        try { return BankWithdrawCmd{true, "", static_cast<uint32_t>(std::stoul(arg("/retirar oro ")))}; }
        catch (...) { return std::nullopt; }
    }
    if (text.starts_with("/clan-rechazar ")) return ClanRejectCmd{arg("/clan-rechazar ")};
    if (text.starts_with("/clan-aceptar "))  return ClanAcceptCmd{arg("/clan-aceptar ")};
    if (text.starts_with("/clan-unban "))    return ClanUnbanCmd{arg("/clan-unban ")};
    if (text.starts_with("/clan-kick "))     return ClanKickCmd{arg("/clan-kick ")};
    if (text.starts_with("/clan-ban "))      return ClanBanCmd{arg("/clan-ban ")};
    if (text.starts_with("/fundar-clan "))   return ClanFoundCmd{arg("/fundar-clan ")};
    if (text.starts_with("/desequipar ")) {
        std::string slot = arg("/desequipar ");
        if (slot == "weapon") return UnequipItemCmd{EquipSlot::WEAPON};
        if (slot == "armor")  return UnequipItemCmd{EquipSlot::ARMOR};
        if (slot == "helmet") return UnequipItemCmd{EquipSlot::HELMET};
        if (slot == "shield") return UnequipItemCmd{EquipSlot::SHIELD};
        return std::nullopt;
    }
    if (text.starts_with("/depositar ")) return BankDepositCmd{false, arg("/depositar "), 0};
    if (text.starts_with("/retirar "))   return BankWithdrawCmd{false, arg("/retirar "), 0};
    if (text.starts_with("/equipar ")) {
        try { return EquipItemCmd{static_cast<uint8_t>(std::stoi(arg("/equipar ")))}; }
        catch (...) { return std::nullopt; }
    }
    if (text.starts_with("/comprar "))  return NpcBuyCmd{arg("/comprar ")};
    if (text.starts_with("/vender "))   return NpcSellCmd{arg("/vender ")};
    if (text.starts_with("/unirse "))   return ClanJoinRequestCmd{arg("/unirse ")};
    if (text.starts_with("/tomar "))    return PickupItemCmd{arg("/tomar ")};
    if (text.starts_with("/tirar "))    return DropItemCmd{arg("/tirar ")};

    return std::nullopt;
}
