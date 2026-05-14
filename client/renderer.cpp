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
        : renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
            background_texture(renderer, load_surface(background.path)),
      background_rect(background.x, background.y, background.width, background.height),
            menu_background_texture(renderer, load_surface("assets/BabelUI/static/media/leather_black..png")),
            menu_logo_texture(renderer, load_surface("assets/BabelUI/static/media/ao20_logo_med..png")),
            menu_button_texture(renderer, load_surface("assets/interface/en_boton-comenzar-default.bmp")),
            menu_background_rect(0, 0, window_w, window_h),
            menu_logo_rect(0, 0, 0, 0),
            menu_button_rect(0, 0, 0, 0),
            window_w(window_w),
        window_h(window_h) {
    init_menu_layout();
    init_sprites(sprites_config);

    if (sprites.empty()) {
        throw std::runtime_error("No sprites to render");
    }
}

void ClientRenderer::init_menu_layout() {
    const int logo_w = menu_logo_texture.GetWidth();
    const int logo_h = menu_logo_texture.GetHeight();
    const int button_w = menu_button_texture.GetWidth();
    const int button_h = menu_button_texture.GetHeight();
    const int logo_x = std::max(0, (window_w - logo_w) / 2);
    const int logo_y = std::max(0, (window_h - logo_h) / 2 - 40);
    const int button_x = std::max(0, (window_w - button_w) / 2);
    const int button_y = std::min(window_h - button_h, logo_y + logo_h + 20);
    menu_logo_rect = SDL2pp::Rect(logo_x, logo_y, logo_w, logo_h);
    menu_button_rect = SDL2pp::Rect(button_x, button_y, button_w, button_h);
}

void ClientRenderer::init_sprites(const std::vector<SpriteConfig>& sprites_config) {
    sprites.reserve(sprites_config.size());
    for (const auto& sprite_config : sprites_config) {
        SpriteRender sprite = build_sprite_render(sprite_config);
        if (sprite.frames.empty()) {
            continue;
        }
        sprites.push_back(std::move(sprite));
    }
}

ClientRenderer::SpriteRender ClientRenderer::build_sprite_render(const SpriteConfig& sprite_config) {
    SpriteRender sprite;
    sprite.frames.reserve(sprite_config.paths.size());
    for (const auto& path : sprite_config.paths) {
        SDL2pp::Surface surface = load_surface(path);
        sprite.frames.emplace_back(renderer, surface);
    }

    if (sprite.frames.empty()) {
        return sprite;
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
    sprite.anchor_to_movable = sprite_config.anchor_to_movable;
    sprite.anchor_offset_x = sprite_config.anchor_offset_x;
    sprite.anchor_offset_y = sprite_config.anchor_offset_y;
    sprite.visible = sprite_config.visible;
    return sprite;
}

void ClientRenderer::render_frame() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear(); // limpa buffer de renderizacion
    renderer.Copy(background_texture, SDL2pp::NullOpt, background_rect);
    update_animation(); // mueve jugador
    update_anchor_positions(); //mueve la cabeza

    for (auto& sprite : sprites) { //for de dibujado
        if (!sprite.visible || sprite.frames.empty()) {
            continue;
        }
        if (sprite.use_src) {
            renderer.Copy(sprite.frames[sprite.current_frame], sprite.src, sprite.dst);
        } else {
            renderer.Copy(sprite.frames[sprite.current_frame], SDL2pp::NullOpt, sprite.dst);
        }
    }

    renderer.Present();
}

void ClientRenderer::render_menu() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(menu_background_texture, SDL2pp::NullOpt, menu_background_rect);
    renderer.Copy(menu_logo_texture, SDL2pp::NullOpt, menu_logo_rect);
    renderer.Copy(menu_button_texture, SDL2pp::NullOpt, menu_button_rect);
    renderer.Present();
}

bool ClientRenderer::is_menu_button_hit(int x, int y) const {
    const int left = menu_button_rect.GetX();
    const int top = menu_button_rect.GetY();
    const int right = left + menu_button_rect.GetW();
    const int bottom = top + menu_button_rect.GetH();
    return x >= left && x <= right && y >= top && y <= bottom;
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

bool ClientRenderer::get_movable_position(int& x, int& y) const {
    const SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return false;
    }
    x = sprite->dst.GetX();
    y = sprite->dst.GetY();
    return true;
}

void ClientRenderer::set_movable_src_y(int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    sprite->src.SetY(y);
}

void ClientRenderer::step_movable_src_x(int step, int frame_count) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    if (frame_count <= 0 || step <= 0) {
        return;
    }
    const int current_index = sprite->src.GetX() / step;
    const int next_index = (current_index + 1) % frame_count;
    sprite->src.SetX(next_index * step);
}

void ClientRenderer::set_anchor_src_y(int y) {
    for (auto& sprite : sprites) {
        if (!sprite.anchor_to_movable || !sprite.use_src) {
            continue;
        }
        sprite.src.SetY(y);
    }
}

SDL2pp::Surface ClientRenderer::load_surface(const std::string& path) {
    SDL_Surface* raw_surface = IMG_Load(path.c_str());
    if (!raw_surface) {
        throw std::runtime_error(std::string("IMG_Load failed: ") + IMG_GetError());
    }
    return SDL2pp::Surface(raw_surface);
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

void ClientRenderer::update_anchor_positions() {
    SpriteRender* movable = find_movable_sprite();
    if (!movable) {
        return;
    }

    for (auto& sprite : sprites) {
        if (!sprite.anchor_to_movable) {
            continue;
        }
        const int desired_x = movable->dst.GetX() + sprite.anchor_offset_x;
        const int desired_y = movable->dst.GetY() + sprite.anchor_offset_y;
        const int clamped_x = std::clamp(desired_x, 0, window_w - sprite.dst.GetW());
        const int clamped_y = std::clamp(desired_y, 0, window_h - sprite.dst.GetH());
        sprite.dst.SetX(clamped_x);
        sprite.dst.SetY(clamped_y);
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

const ClientRenderer::SpriteRender* ClientRenderer::find_movable_sprite() const {
    for (const auto& sprite : sprites) {
        if (sprite.movable) {
            return &sprite;
        }
    }
    return nullptr;
}

