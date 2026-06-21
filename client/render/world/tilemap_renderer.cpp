#include "tilemap_renderer.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "../gfx/texture_loader.h"
#include "viewport.h"

TilemapRenderer::TilemapRenderer(SDL2pp::Renderer& renderer): renderer(renderer) {}

void TilemapRenderer::load(const TilemapConfig& tilemap) {
    if (tilemap.path.empty() || tilemap.mapa.empty() || tilemap.tiles.empty()) {
        loaded_ = false;
        return;
    }

    loaded_ = true;
    tile_size_ = tilemap.tile_size;
    tiles.clear();
    textures.clear();
    px_w = 0;
    px_h = 0;

    for (const auto& row: tilemap.mapa) {
        std::vector<TileSrcInfo> tile_row;
        tile_row.reserve(row.size());
        px_w = std::max(px_w, static_cast<int>(row.size()) * tile_size_);
        for (const auto& name: row) {
            auto it = tilemap.tiles.find(name);
            if (it == tilemap.tiles.end()) {
                tile_row.push_back({{0, 0, tile_size_, tile_size_}, tilemap.path});
                continue;
            }
            const TileDef& def = it->second;
            std::string atlas_path = def.path.empty() ? tilemap.path : def.path;
            tile_row.push_back({{def.x, def.y, tile_size_, tile_size_}, atlas_path});

            if (textures.find(atlas_path) == textures.end()) {
                textures.emplace(atlas_path,
                                 SDL2pp::Texture(renderer, texture::load_surface(atlas_path)));
            }
        }
        tiles.push_back(std::move(tile_row));
    }

    px_h = static_cast<int>(tiles.size()) * tile_size_;
}

void TilemapRenderer::render(const SDL2pp::Rect& cam) {
    if (!loaded_)
        return;

    auto [first_col, first_row, last_col, last_row] = compute_visible_range(cam, tile_size_, 8);

    for (int row = first_row; row <= last_row; ++row) {
        if (row < 0 || row >= static_cast<int>(tiles.size()))
            continue;

        const auto& tile_row = tiles[static_cast<std::size_t>(row)];
        const int row_last_col = std::min(last_col, static_cast<int>(tile_row.size()) - 1);
        for (int col = first_col; col <= row_last_col; ++col) {
            if (col < 0)
                continue;

            const auto& tile = tile_row[static_cast<std::size_t>(col)];
            auto tex_it = textures.find(tile.atlas_path);
            if (tex_it == textures.end())
                continue;

            SDL2pp::Rect dst(col * tile_size_ - cam.GetX(), row * tile_size_ - cam.GetY(),
                             tile_size_, tile_size_);
            renderer.Copy(tex_it->second, tile.src, dst);
        }
    }
}
