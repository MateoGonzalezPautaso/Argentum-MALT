#ifndef PLAYER_REGISTRY_H
#define PLAYER_REGISTRY_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "player.h"

// Envuelve la coleccion de jugadores del Game y centraliza las queries de
// dominio sobre ella (por ejemplo, ids de jugadores en un mapa dado) para
// evitar reimplementarlas en cada servicio que recibe `players` por
// referencia.
class PlayerRegistry {
public:
    explicit PlayerRegistry(const std::map<uint16_t, Player>& players);

    std::vector<uint16_t> ids_on_map(const std::string& map_name) const;

private:
    const std::map<uint16_t, Player>& players_;
};

#endif  // PLAYER_REGISTRY_H
