#ifndef CLIENT_RENDER_CONTEXT_H
#define CLIENT_RENDER_CONTEXT_H

#include <string>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

struct TTFGuard {
    TTFGuard();
    ~TTFGuard();
};

struct ImageGuard {
    explicit ImageGuard(int flags);
    ~ImageGuard();
};

class RenderContext {
public:
    RenderContext(const std::string& title, int window_w, int window_h, int logical_w = 1024,
                  int logical_h = 768);
    ~RenderContext();

    SDL2pp::Renderer& renderer() { return sdl_renderer; }

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

private:
    SDL2pp::SDL sdl;
    TTFGuard ttf;
    ImageGuard img;
    SDL2pp::Window window;
    SDL2pp::Renderer sdl_renderer;
    int logical_w;
    int logical_h;
};

#endif
