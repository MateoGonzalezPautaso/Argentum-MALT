#include "texture_loader.h"

#include <stdexcept>

#include <SDL2/SDL_image.h>

namespace texture {

SDL2pp::Surface load_surface(const std::string& path) {
    SDL_Surface* raw = IMG_Load(path.c_str());
    if (!raw) {
        throw std::runtime_error(std::string("IMG_Load failed: ") + IMG_GetError());
    }
    return SDL2pp::Surface(raw);
}

}  // namespace texture
