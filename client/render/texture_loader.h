#ifndef CLIENT_TEXTURE_LOADER_H
#define CLIENT_TEXTURE_LOADER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>

namespace texture {
SDL2pp::Surface load_surface(const std::string& path);
}

#endif  // CLIENT_TEXTURE_LOADER_H
