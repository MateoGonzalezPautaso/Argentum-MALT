#include "map_visuals_config.h"

#include <filesystem>

#include <toml++/toml.h>

MapVisualCatalog load_map_visual_catalog(const std::string& path) {
    toml::table root = toml::parse_file(path);
    MapVisualCatalog catalog;

    if (auto tilemap = root["tilemap"].as_table()) {
        catalog.tilemap_path = toml_get_string(*tilemap, "path", std::string());
        if (auto tiles = (*tilemap)["tiles"].as_table())
            parse_tile_definitions(*tiles, catalog.tile_visuals);
    }

    if (auto prop = root["prop"].as_table()) {
        if (auto prop_tiles = (*prop)["tiles"].as_table())
            parse_prop_definitions(*prop_tiles, catalog.prop_visuals);
    }

    return catalog;
}

std::unordered_map<std::string, MapVisualCatalog> load_all_map_visual_catalogs(
        const std::string& visuals_dir) {
    namespace fs = std::filesystem;
    std::unordered_map<std::string, MapVisualCatalog> result;

    std::error_code ec;
    if (!fs::is_directory(visuals_dir, ec))
        return result;

    for (const auto& entry: fs::directory_iterator(visuals_dir)) {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".toml")
            continue;
        std::string map_name = entry.path().stem().string();
        result.emplace(map_name, load_map_visual_catalog(entry.path().string()));
    }

    return result;
}
