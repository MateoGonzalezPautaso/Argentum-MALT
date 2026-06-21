#ifndef CLIENT_TEXTURE_CACHE_H
#define CLIENT_TEXTURE_CACHE_H

#include <string>
#include <unordered_map>

#include <SDL2pp/SDL2pp.hh>

class TextureCache {
public:
    explicit TextureCache(SDL2pp::Renderer& renderer);

    // Devuelve la textura cargada desde `path`, cargándola si no estaba en caché.
    SDL2pp::Texture& get(const std::string& path);

    void clear();
private:
    SDL2pp::Renderer& renderer_;
    std::unordered_map<std::string, SDL2pp::Texture> cache_;
};

#endif  // CLIENT_TEXTURE_CACHE_H
