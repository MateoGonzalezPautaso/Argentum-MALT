#ifndef MAP_DATA_SERVICE_H
#define MAP_DATA_SERVICE_H

#include <string>
#include <unordered_map>

#include "../command_result.h"
#include "../map.h"

/*
 * MapDataService
 *
 * Responsabilidad única: responder pedidos de descarga de mapas del cliente
 * (REQUEST_MAP_DATA). Convierte el Map del servidor en un MapDataEvent listo para enviar
 *
 * Sigue el patrón de los demás servicios extraídos de Game: recibe sus
 * dependencias por referencia (inyección de dependencias) y no acopla con Game.
 */
class MapDataService {
public:
    explicit MapDataService(const std::unordered_map<std::string, Map>& maps);

    // Devuelve el MapDataEvent como evento privado para el jugador que lo pidió.
    // Si el mapa no existe, devuelve un CommandResult vacío (el cliente
    // simplemente no recibe respuesta).
    CommandResult handle_request(const std::string& map_name) const;

private:
    const std::unordered_map<std::string, Map>& maps_;
};

#endif  // MAP_DATA_SERVICE_H
