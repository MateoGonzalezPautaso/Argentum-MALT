#include "renderer.h"

#include <algorithm>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

ClientRenderer::ClientRenderer(SDL2pp::Window& window,
                               const BackgroundConfig& background,
                               const std::vector<SpriteConfig>& sprites_config,
                               int window_w,
                               int window_h)
    : window(window),
      renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      background_surface(load_surface(background.path)),
      background_texture(renderer, background_surface),
      background_rect(background.x, background.y, background.width, background.height),
      window_w(window_w),
      window_h(window_h),
      show_extra_sprite(false) {
    sprites.reserve(sprites_config.size());
    for (const auto& sprite_config : sprites_config) {
        SpriteRender sprite;
        sprite.frames.reserve(sprite_config.paths.size());
        for (const auto& path : sprite_config.paths) {
            SDL2pp::Surface surface = load_surface(path);
            sprite.frames.emplace_back(renderer, surface);
        }

        if (sprite.frames.empty()) {
            continue;
        }

        sprite.dst = SDL2pp::Rect(sprite_config.x,
                                  sprite_config.y,
                                  sprite_config.width,
                                  sprite_config.height);
        if (sprite_config.src_width > 0 && sprite_config.src_height > 0) {
            sprite.src = SDL2pp::Rect(sprite_config.src_x,
                                      sprite_config.src_y,
                                      sprite_config.src_width,
                                      sprite_config.src_height);
            sprite.use_src = true;
        }
        sprite.animated = sprite.frames.size() > 1;
        sprite.frame_ms = sprite_config.frame_ms;
        sprite.current_frame = 0;
        sprite.last_ticks = SDL_GetTicks();
        sprite.movable = sprite_config.movable;
        sprite.visible = sprite_config.visible;
        sprites.push_back(std::move(sprite));
    }

    if (sprites.empty()) {
        throw std::runtime_error("No sprites to render");
    }
}

void ClientRenderer::render_frame() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(background_texture, SDL2pp::NullOpt, background_rect);
    update_animation();

    for (auto& sprite : sprites) {
        if (!sprite.visible || sprite.frames.empty()) {
            continue;
        }
        if (sprite.use_src) {
            renderer.Copy(sprite.frames[sprite.current_frame], sprite.src, sprite.dst);
        } else {
            renderer.Copy(sprite.frames[sprite.current_frame], SDL2pp::NullOpt, sprite.dst);
        }
    }

    if (show_extra_sprite && !sprites.empty()) {
        auto& sprite = sprites.front();
        if (sprite.use_src) {
            renderer.Copy(sprite.frames[sprite.current_frame], sprite.src, extra_sprite_dst(sprite));
        } else {
            renderer.Copy(sprite.frames[sprite.current_frame], SDL2pp::NullOpt, extra_sprite_dst(sprite));
        }
    }

    renderer.Present();
}

void ClientRenderer::move_sprite(int dx, int dy) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return;
    }
    const int new_x = std::clamp(sprite->dst.GetX() + dx, 0, window_w - sprite->dst.GetW());
    const int new_y = std::clamp(sprite->dst.GetY() + dy, 0, window_h - sprite->dst.GetH());
    sprite->dst.SetX(new_x);
    sprite->dst.SetY(new_y);
}

void ClientRenderer::toggle_extra_sprite() {
    show_extra_sprite = !show_extra_sprite;
}

void ClientRenderer::set_movable_src_y(int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    sprite->src.SetY(y);
}

void ClientRenderer::step_movable_src_x(int step, int max_x) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    int next = sprite->src.GetX() + step;
    if (next > max_x) {
        next = 0;
    }
    sprite->src.SetX(next);
}

SDL2pp::Surface ClientRenderer::load_surface(const std::string& path) {
    SDL_Surface* raw_surface = IMG_Load(path.c_str());
    if (!raw_surface) {
        throw std::runtime_error(std::string("IMG_Load failed: ") + IMG_GetError());
    }
    return SDL2pp::Surface(raw_surface);
}

SDL2pp::Rect ClientRenderer::extra_sprite_dst(const SpriteRender& sprite) const {
    const int offset = sprite.dst.GetW() + 10;
    const int new_x = std::min(sprite.dst.GetX() + offset, window_w - sprite.dst.GetW());
    return SDL2pp::Rect(new_x, sprite.dst.GetY(), sprite.dst.GetW(), sprite.dst.GetH());
}

void ClientRenderer::update_animation() {
    const uint32_t now = SDL_GetTicks();
    for (auto& sprite : sprites) {
        if (!sprite.animated || sprite.frame_ms == 0) {
            continue;
        }
        if (now - sprite.last_ticks < sprite.frame_ms) {
            continue;
        }
        sprite.last_ticks = now;
        sprite.current_frame = (sprite.current_frame + 1) % sprite.frames.size();
    }
}

ClientRenderer::SpriteRender* ClientRenderer::find_movable_sprite() {
    for (auto& sprite : sprites) {
        if (sprite.movable) {
            return &sprite;
        }
    }
    return nullptr;
}

