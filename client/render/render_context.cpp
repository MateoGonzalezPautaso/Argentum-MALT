#include "render_context.h"

#include <string>

RenderContext::RenderContext(const std::string& title, int window_w, int window_h, int logical_w,
                             int logical_h):
        sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
        window(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_w, window_h, 0),
        sdl_renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        logical_w(logical_w),
        logical_h(logical_h) {
    SDL_RenderSetLogicalSize(sdl_renderer.Get(), logical_w, logical_h);
}

RenderContext::~RenderContext() = default;
