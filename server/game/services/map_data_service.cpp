#include "map_data_service.h"

#include "../map_level_data_builder.h"

MapDataService::MapDataService(const std::unordered_map<std::string, Map>& maps): maps_(maps) {}

CommandResult MapDataService::handle_request(const std::string& map_name) const {
    auto it = maps_.find(map_name);
    if (it == maps_.end())
        return {};

    MapDataEvent ev{MapLevelDataBuilder::build(map_name, it->second)};
    return {.private_events = {std::move(ev)}};
}
