#ifndef EDITOR_ATLAS_LOADER_H
#define EDITOR_ATLAS_LOADER_H

#include <QPixmap>
#include <string>
#include <unordered_map>
#include "common/config.h"

class AtlasLoader {
public:
    void load(const TilemapConfig& config);
    const QPixmap* get(const std::string& path) const;
    const std::unordered_map<std::string, QPixmap>& all() const { return atlases_; }
    void clear() { atlases_.clear(); }

private:
    std::unordered_map<std::string, QPixmap> atlases_;
};

#endif
