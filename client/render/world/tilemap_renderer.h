#ifndef CLIENT_TILEMAP_RENDERER_H
#define CLIENT_TILEMAP_RENDERER_H

#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>

#include "../../config/config.h"

class TilemapRenderer {
public:
    explicit TilemapRenderer(SDL2pp::Renderer& renderer);

    void load(const TilemapConfig& tilemap);
    void render(const SDL2pp::Rect& cam);

    int pixel_width() const { return px_w; }
    int pixel_height() const { return px_h; }
    int tile_size() const { return tile_size_; }
    bool is_loaded() const { return loaded_; }

private:
    struct TileSrcInfo {
        SDL2pp::Rect src;
        std::string atlas_path;
    };

    SDL2pp::Renderer& renderer;
    std::unordered_map<std::string, SDL2pp::Texture> textures;
    std::vector<std::vector<TileSrcInfo>> tiles;
    int tile_size_ = 128;
    int px_w = 0;
    int px_h = 0;
    bool loaded_ = false;
};

#endif
