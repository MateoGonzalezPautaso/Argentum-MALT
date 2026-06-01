#include "render_context.h"

#include <stdexcept>
#include <string>

#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>

TTFGuard::TTFGuard() {
    if (TTF_Init() != 0)
        throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
}
TTFGuard::~TTFGuard() { TTF_Quit(); }

ImageGuard::ImageGuard(int flags) {
    if ((IMG_Init(flags) & flags) != flags)
        throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
}
ImageGuard::~ImageGuard() { IMG_Quit(); }

RenderContext::RenderContext(const std::string& title, int window_w, int window_h, int logical_w,
                             int logical_h, bool fullscreen):
        sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
        ttf(),
        img(IMG_INIT_PNG),
        window(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_w, window_h,
               fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0),
        sdl_renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        logical_w(logical_w),
        logical_h(logical_h) {
    SDL_RenderSetLogicalSize(sdl_renderer.Get(), logical_w, logical_h);
}

RenderContext::~RenderContext() = default;
