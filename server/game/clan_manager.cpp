#include "clan_manager.h"

#include <algorithm>
#include <utility>

ClanManager::ClanManager(ClanPersistence& persistence, const ClanConfig& config):
        persistence(persistence), config(config) {
    auto loaded = persistence.load_all();
    for (auto& cd: loaded) {
        ClanData clan;
        clan.name = std::move(cd.clan_name);
        clan.founder = std::move(cd.founder_username);
        clan.members = std::move(cd.members);
        clan.banned = {cd.banned.begin(), cd.banned.end()};

        for (const auto& m: clan.members) {
            player_to_clan[m] = clan.name;
        }

        clans.emplace(clan.name, std::move(clan));
    }
}

ClanResult ClanManager::create_clan(const std::string& founder_username,
                                    const std::string& clan_name) {
    if (clan_name.empty()) {
        return {false, "El nombre del clan no puede estar vacio"};
    }

    if (clans.find(clan_name) != clans.end()) {
        return {false, "Ya existe un clan con el nombre " + clan_name};
    }

    if (player_to_clan.find(founder_username) != player_to_clan.end()) {
        return {false, "Ya perteneces a un clan"};
    }

    ClanData clan;
    clan.name = clan_name;
    clan.founder = founder_username;
    clan.members.push_back(founder_username);

    player_to_clan[founder_username] = clan_name;
    clans.emplace(clan_name, std::move(clan));

    auto it = clans.find(clan_name);
    if (it != clans.end()) {
        save_clan(it->second);
    }

    return {true, "Clan " + clan_name + " fundado exitosamente"};
}

ClanResult ClanManager::request_join(const std::string& applicant_username,
                                     const std::string& clan_name) {
    if (player_to_clan.find(applicant_username) != player_to_clan.end()) {
        return {false, "Ya perteneces a un clan"};
    }

    auto it = clans.find(clan_name);
    if (it == clans.end()) {
        return {false, "El clan " + clan_name + " no existe"};
    }

    ClanData& clan = it->second;

    if (clan.banned.find(applicant_username) != clan.banned.end()) {
        return {false, "Estas baneado de este clan"};
    }

    if (static_cast<int>(clan.members.size()) >= config.max_members) {
        return {false,
                "El clan esta lleno (maximo " + std::to_string(config.max_members) + " miembros)"};
    }

    auto& requests = clan.join_requests;
    if (std::find(requests.begin(), requests.end(), applicant_username) != requests.end()) {
        return {false, "Ya tienes un pedido pendiente para este clan"};
    }

    requests.push_back(applicant_username);
    return {true, "Solicitud enviada al clan " + clan_name};
}

ClanResult ClanManager::accept_member(const std::string& founder_username,
                                      const std::string& target_username) {
    auto* clan = find_clan_by_member(founder_username);
    if (!clan) {
        return {false, "No perteneces a ningun clan"};
    }

    if (clan->founder != founder_username) {
        return {false, "Solo el fundador puede aceptar miembros"};
    }

    auto& requests = clan->join_requests;
    auto req_it = std::find(requests.begin(), requests.end(), target_username);
    if (req_it == requests.end()) {
        return {false, target_username + " no tiene un pedido pendiente"};
    }

    if (static_cast<int>(clan->members.size()) >= config.max_members) {
        return {false, "El clan esta lleno"};
    }

    requests.erase(req_it);
    add_member_to_clan(*clan, target_username);
    return {true, target_username + " ha sido aceptado en el clan"};
}

ClanResult ClanManager::reject_member(const std::string& founder_username,
                                      const std::string& target_username) {
    auto* clan = find_clan_by_member(founder_username);
    if (!clan) {
        return {false, "No perteneces a ningun clan"};
    }

    if (clan->founder != founder_username) {
        return {false, "Solo el fundador puede rechazar miembros"};
    }

    auto& requests = clan->join_requests;
    auto req_it = std::find(requests.begin(), requests.end(), target_username);
    if (req_it == requests.end()) {
        return {false, target_username + " no tiene un pedido pendiente"};
    }

    requests.erase(req_it);
    return {true, target_username + " ha sido rechazado"};
}

ClanResult ClanManager::ban_member(const std::string& founder_username,
                                   const std::string& target_username) {
    auto* clan = find_clan_by_member(founder_username);
    if (!clan) {
        return {false, "No perteneces a ningun clan"};
    }

    if (clan->founder != founder_username) {
        return {false, "Solo el fundador puede banear miembros"};
    }

    if (target_username == founder_username) {
        return {false, "No puedes banearte a ti mismo"};
    }

    auto& requests = clan->join_requests;
    auto req_it = std::find(requests.begin(), requests.end(), target_username);
    if (req_it != requests.end()) {
        requests.erase(req_it);
    }

    // Also kick if already a member
    auto mem_it = std::find(clan->members.begin(), clan->members.end(), target_username);
    if (mem_it != clan->members.end()) {
        clan->members.erase(mem_it);
        player_to_clan.erase(target_username);
    }

    clan->banned.insert(target_username);
    save_clan(*clan);
    return {true, target_username + " ha sido baneado del clan"};
}

ClanResult ClanManager::kick_member(const std::string& founder_username,
                                    const std::string& target_username) {
    auto* clan = find_clan_by_member(founder_username);
    if (!clan) {
        return {false, "No perteneces a ningun clan"};
    }

    if (clan->founder != founder_username) {
        return {false, "Solo el fundador puede expulsar miembros"};
    }

    if (target_username == founder_username) {
        return {false, "No puedes expulsarte a ti mismo"};
    }

    auto mem_it = std::find(clan->members.begin(), clan->members.end(), target_username);
    if (mem_it == clan->members.end()) {
        return {false, target_username + " no es miembro del clan"};
    }

    clan->members.erase(mem_it);
    player_to_clan.erase(target_username);
    save_clan(*clan);
    return {true, target_username + " ha sido expulsado del clan"};
}

ClanResult ClanManager::leave_clan(const std::string& username) {
    auto* clan = find_clan_by_member(username);
    if (!clan) {
        return {false, "No perteneces a ningun clan"};
    }

    if (clan->founder == username) {
        return {false, "El fundador no puede irse del clan"};
    }

    auto mem_it = std::find(clan->members.begin(), clan->members.end(), username);
    if (mem_it != clan->members.end()) {
        clan->members.erase(mem_it);
    }

    player_to_clan.erase(username);
    save_clan(*clan);
    return {true, "Has abandonado el clan " + clan->name};
}

bool ClanManager::is_in_clan(const std::string& username) const {
    return player_to_clan.find(username) != player_to_clan.end();
}

bool ClanManager::is_founder(const std::string& username) const {
    auto it = player_to_clan.find(username);
    if (it == player_to_clan.end())
        return false;
    auto cit = clans.find(it->second);
    return cit != clans.end() && cit->second.founder == username;
}

bool ClanManager::are_in_same_clan(const std::string& u1, const std::string& u2) const {
    if (u1 == u2)
        return false;
    auto it1 = player_to_clan.find(u1);
    if (it1 == player_to_clan.end())
        return false;
    auto it2 = player_to_clan.find(u2);
    if (it2 == player_to_clan.end())
        return false;
    return it1->second == it2->second;
}

std::string ClanManager::get_clan_name(const std::string& username) const {
    auto it = player_to_clan.find(username);
    if (it == player_to_clan.end())
        return {};
    return it->second;
}

std::vector<ClanMember> ClanManager::get_member_list(const std::string& clan_name) const {
    std::vector<ClanMember> result;
    auto it = clans.find(clan_name);
    if (it == clans.end())
        return result;

    const ClanData& clan = it->second;
    for (const auto& m: clan.members) {
        ClanMember member;
        member.username = m;
        member.is_founder = (m == clan.founder);
        member.is_online = true;  // Will be updated by caller
        result.push_back(std::move(member));
    }
    return result;
}

std::vector<std::string> ClanManager::get_pending_requests(const std::string& clan_name) const {
    auto it = clans.find(clan_name);
    if (it == clans.end())
        return {};
    return it->second.join_requests;
}

bool ClanManager::has_pending_request(const std::string& username,
                                      const std::string& clan_name) const {
    auto it = clans.find(clan_name);
    if (it == clans.end())
        return false;
    const auto& requests = it->second.join_requests;
    return std::find(requests.begin(), requests.end(), username) != requests.end();
}

ClanManager::ClanData* ClanManager::find_clan_by_member(const std::string& username) {
    auto it = player_to_clan.find(username);
    if (it == player_to_clan.end())
        return nullptr;
    auto cit = clans.find(it->second);
    if (cit == clans.end())
        return nullptr;
    return &cit->second;
}

const ClanManager::ClanData* ClanManager::find_clan_by_name(const std::string& clan_name) const {
    auto it = clans.find(clan_name);
    if (it == clans.end())
        return nullptr;
    return &it->second;
}

void ClanManager::add_member_to_clan(ClanData& clan, const std::string& username) {
    clan.members.push_back(username);
    player_to_clan[username] = clan.name;
    save_clan(clan);
}

void ClanManager::save_clan(const ClanData& clan) {
    ClanPersistence::ClanData data;
    data.clan_name = clan.name;
    data.founder_username = clan.founder;
    data.members = clan.members;
    data.banned.assign(clan.banned.begin(), clan.banned.end());
    persistence.save_one(clan.name, data);
}
