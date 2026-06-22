#include "texture_cache.h"

#include "texture_loader.h"

TextureCache::TextureCache(SDL2pp::Renderer& renderer): renderer_(renderer) {}

SDL2pp::Texture& TextureCache::get(const std::string& path) {
    auto it = cache_.find(path);
    if (it == cache_.end()) {
        auto result =
                cache_.try_emplace(path, SDL2pp::Texture(renderer_, texture::load_surface(path)));
        it = result.first;
    }
    return it->second;
}

void TextureCache::clear() { cache_.clear(); }
