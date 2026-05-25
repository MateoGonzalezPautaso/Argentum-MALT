#include "atlas_loader.h"

void AtlasLoader::load(const TilemapConfig& config) {
    auto load_if_missing = [this](const std::string& path) {
        if (!path.empty() && atlases_.find(path) == atlases_.end()) {
            atlases_.emplace(path, QPixmap(QString::fromStdString(path)));
        }
    };
    load_if_missing(config.path);
    for (const auto& [name, def]: config.tiles) {
        load_if_missing(def.path);
    }
    for (const auto& [name, def]: config.props) {
        if (!def.paths.empty()) {
            load_if_missing(def.paths[0]);
        }
    }
}

const QPixmap* AtlasLoader::get(const std::string& path) const {
    auto it = atlases_.find(path);
    if (it != atlases_.end() && !it->second.isNull()) {
        return &it->second;
    }
    return nullptr;
}
