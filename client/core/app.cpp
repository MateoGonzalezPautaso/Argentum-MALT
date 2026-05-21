#include "app.h"

#include <stdexcept>

#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>

namespace client_app {

void init_image() {
    const int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
    }
}

void shutdown_image() { IMG_Quit(); }

void init_ttf() {
    if (TTF_Init() != 0) {
        throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
    }
}

void shutdown_ttf() { TTF_Quit(); }

ClientConfig load_config() {
    const std::string config_path = "config/client.toml";
    return load_client_config(config_path);
}

}  // namespace client_app
