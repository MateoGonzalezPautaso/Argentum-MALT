#ifndef EDITOR_TOML_SERIALIZER_H
#define EDITOR_TOML_SERIALIZER_H

#include <string>

#include "common/config.h"

class TomlSerializer {
public:
    static TilemapConfig load(const std::string& path);
    static void save(const std::string& path, const TilemapConfig& config);
};

#endif
