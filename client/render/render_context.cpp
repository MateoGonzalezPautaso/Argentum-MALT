#include "render_context.h"

RenderContext::RenderContext(const std::string& title, int window_w, int window_h):
        sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
        window(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_w, window_h, 0),
        sdl_renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) {
    SDL_RenderSetLogicalSize(sdl_renderer.Get(), LOGICAL_W, LOGICAL_H);
}

RenderContext::~RenderContext() = default;
