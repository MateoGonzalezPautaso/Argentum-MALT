#ifndef CLIENT_RENDER_CONTEXT_H
#define CLIENT_RENDER_CONTEXT_H

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

class RenderContext {
public:
    static constexpr int LOGICAL_W = 1024;
    static constexpr int LOGICAL_H = 768;

    RenderContext(const std::string& title, int window_w, int window_h);
    ~RenderContext();

    SDL2pp::Renderer& renderer() { return sdl_renderer; }

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

private:
    SDL2pp::SDL sdl;
    SDL2pp::Window window;
    SDL2pp::Renderer sdl_renderer;
};

#endif
