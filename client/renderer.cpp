#include "renderer.h"

#include <algorithm>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

ClientRenderer::ClientRenderer(SDL2pp::Window& window,
                               const BackgroundConfig& background,
                               const TilemapConfig& tilemap,
                               const std::vector<SpriteConfig>& sprites_config,
                               int window_w,
                               int window_h)
        : renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
            background_texture(renderer, load_surface(background.path)),
            background_rect(background.x, background.y, background.width, background.height),
            ui_frame_texture(renderer, load_surface("assets/interface/en_ventanaprincipal.bmp")),
            ui_frame_rect(0, 0, window_w, window_h),
            game_viewport(11, 149, 734, 608),
            tilemap_texture(renderer, load_surface(tilemap.path.empty() ? background.path : tilemap.path)),
            menu_background_texture(renderer, load_surface("assets/BabelUI/static/media/leather_black..png")),
            menu_logo_texture(renderer, load_surface("assets/BabelUI/static/media/ao20_logo_med..png")),
            menu_button_texture(renderer, load_surface("assets/interface/en_boton-comenzar-default.bmp")),
            menu_background_rect(0, 0, window_w, window_h),
            menu_logo_rect(0, 0, 0, 0),
            menu_button_rect(0, 0, 0, 0),
            window_w(window_w),
        window_h(window_h) {
    init_tilemap(tilemap);
    init_menu_layout();
    init_sprites(sprites_config);

    if (sprites.empty()) {
        throw std::runtime_error("No sprites to render");
    }
}

void ClientRenderer::init_menu_layout() {
    menu_background_rect = SDL2pp::Rect(0, 0, window_w, window_h);

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

void ClientRenderer::init_tilemap(const TilemapConfig& tilemap) {
    if (tilemap.path.empty() || tilemap.mapa.empty() || tilemap.tiles.empty()) {
        has_tilemap = false;
        return;
    }

    has_tilemap = true;
    tile_size = tilemap.tile_size;
    tilemap_src.clear();
    tilemap_walkable.clear();
    tilemap_src.reserve(tilemap.mapa.size());
    tilemap_walkable.reserve(tilemap.mapa.size());
    map_px_w = 0;
    map_px_h = 0;

    for (const auto& row : tilemap.mapa) {
        std::vector<SDL2pp::Rect> src_row;
        std::vector<bool> walkable_row;
        src_row.reserve(row.size());
        walkable_row.reserve(row.size());
        map_px_w = std::max(map_px_w, static_cast<int>(row.size()) * tile_size);
        for (const auto& name : row) {
            auto it = tilemap.tiles.find(name);
            if (it == tilemap.tiles.end()) {
                src_row.emplace_back(0, 0, tile_size, tile_size);
                walkable_row.push_back(false);
                continue;
            }
            const TileDef& def = it->second;
            src_row.emplace_back(def.x, def.y, tile_size, tile_size);
            walkable_row.push_back(def.walkable);
        }
        tilemap_src.push_back(std::move(src_row));
        tilemap_walkable.push_back(std::move(walkable_row));
    }

    map_px_h = static_cast<int>(tilemap_src.size()) * tile_size;
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
    renderer.Clear(); // limpia el backbuffer

    renderer.Copy(ui_frame_texture, SDL2pp::NullOpt, ui_frame_rect);
    renderer.SetViewport(game_viewport);

    const SDL2pp::Rect cam = camera_rect();
    render_tilemap_or_background(cam);
    update_animation(); // avanza sprites animados
    update_anchor_positions(); // sincroniza sprites anclados

    render_sprites(cam);
    renderer.SetViewport(SDL2pp::NullOpt);

    renderer.Present();
}

void ClientRenderer::render_tilemap_or_background(const SDL2pp::Rect& cam) {
    if (has_tilemap) {
        for (std::size_t row = 0; row < tilemap_src.size(); ++row) {
            const auto& src_row = tilemap_src[row];
            for (std::size_t col = 0; col < src_row.size(); ++col) {
                SDL2pp::Rect dst(static_cast<int>(col) * tile_size - cam.GetX(),
                                 static_cast<int>(row) * tile_size - cam.GetY(),
                                 tile_size,
                                 tile_size);
                renderer.Copy(tilemap_texture, src_row[col], dst);
            }
        }
        return;
    }

    const SDL2pp::Rect gameplay_bg(0, 0, game_viewport.GetW(), game_viewport.GetH());
    renderer.Copy(background_texture, SDL2pp::NullOpt, gameplay_bg);
}

void ClientRenderer::render_sprites(const SDL2pp::Rect& cam) {
    // Dibuja sprites visibles en el orden configurado.
    for (auto& sprite : sprites) {
        if (!sprite.visible || sprite.frames.empty()) {
            continue;
        }
        SDL2pp::Rect dst(sprite.dst.GetX() - cam.GetX(),
                         sprite.dst.GetY() - cam.GetY(),
                         sprite.dst.GetW(),
                         sprite.dst.GetH());
        if (sprite.use_src) {
            renderer.Copy(sprite.frames[sprite.current_frame], sprite.src, dst);
        } else {
            renderer.Copy(sprite.frames[sprite.current_frame], SDL2pp::NullOpt, dst);
        }
    }
}

SDL2pp::Rect ClientRenderer::camera_rect() const {
    const SpriteRender* sprite = find_movable_sprite();
    int cam_x = 0;
    int cam_y = 0;
    if (sprite) {
        cam_x = sprite->dst.GetX() + sprite->dst.GetW() / 2 - game_viewport.GetW() / 2;
        cam_y = sprite->dst.GetY() + sprite->dst.GetH() / 2 - game_viewport.GetH() / 2;
    }

    if (has_tilemap) {
        const int max_x = std::max(0, map_px_w - game_viewport.GetW());
        const int max_y = std::max(0, map_px_h - game_viewport.GetH());
        cam_x = std::clamp(cam_x, 0, max_x);
        cam_y = std::clamp(cam_y, 0, max_y);
    } else {
        cam_x = std::max(0, cam_x);
        cam_y = std::max(0, cam_y);
    }

    return SDL2pp::Rect(cam_x, cam_y, game_viewport.GetW(), game_viewport.GetH());
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
    const int max_x = has_tilemap ? std::max(0, map_px_w - sprite->dst.GetW())
                                  : std::max(0, window_w - sprite->dst.GetW());
    const int max_y = has_tilemap ? std::max(0, map_px_h - sprite->dst.GetH())
                                  : std::max(0, window_h - sprite->dst.GetH());
    const int new_x = std::clamp(sprite->dst.GetX() + dx, 0, max_x);
    const int new_y = std::clamp(sprite->dst.GetY() + dy, 0, max_y);

    if (!is_walkable_for_sprite(new_x, new_y, *sprite)) {
        return;
    }

    sprite->dst.SetX(new_x);
    sprite->dst.SetY(new_y);
}

bool ClientRenderer::is_walkable_for_sprite(int x, int y, const SpriteRender& sprite) const {
    if (!has_tilemap) {
        return true;
    }
    if (tile_size <= 0 || tilemap_walkable.empty()) {
        return true;
    }

    const int foot_x = x + sprite.dst.GetW() / 2;
    const int foot_y = y + sprite.dst.GetH() - 1;
    if (foot_x < 0 || foot_y < 0) {
        return false;
    }

    const int col = foot_x / tile_size;
    const int row = foot_y / tile_size;
    if (row < 0 || row >= static_cast<int>(tilemap_walkable.size())) {
        return false;
    }
    if (col < 0 || col >= static_cast<int>(tilemap_walkable[row].size())) {
        return false;
    }

    return tilemap_walkable[row][col];
}

bool ClientRenderer::get_movable_position(int& x, int& y) const {
    // sirve para obtener la posicion actual del sprite movible, para el movimiento por click
    const SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return false;
    }
    x = sprite->dst.GetX();
    y = sprite->dst.GetY();
    return true;
}

void ClientRenderer::get_camera_offset(int& x, int& y) const {
    const SDL2pp::Rect cam = camera_rect();
    x = cam.GetX();
    y = cam.GetY();
}

bool ClientRenderer::screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const {
    const int local_x = screen_x - game_viewport.GetX();
    const int local_y = screen_y - game_viewport.GetY();
    if (local_x < 0 || local_y < 0 || local_x >= game_viewport.GetW() || local_y >= game_viewport.GetH()) {
        return false;
    }

    int cam_x = 0;
    int cam_y = 0;
    get_camera_offset(cam_x, cam_y);
    world_x = local_x + cam_x;
    world_y = local_y + cam_y;
    return true;
}

void ClientRenderer::set_movable_src_y(int y) {
    // sirve para cambiar la fila en la imagen del sprite movible
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    sprite->src.SetY(y);
}

void ClientRenderer::step_movable_src_x(int step, int frame_count) {
    // sirve para avanzar la animacion de caminata en la imagen del sprite movible
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
    // junta dos sprites usando la misma fila (para agregar la cabeza del personaje)
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
    // avanza la animacion de los sprites segun su configuracion de frame_ms.
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
    // refresh de posiciones de sprites anclados al sprite movible, para que lo sigan en cada movimiento.
    SpriteRender* movable = find_movable_sprite();
    if (!movable) {
        return;
    }

    const int max_x = has_tilemap ? std::max(0, map_px_w - movable->dst.GetW())
                                  : std::max(0, window_w - movable->dst.GetW());
    const int max_y = has_tilemap ? std::max(0, map_px_h - movable->dst.GetH())
                                  : std::max(0, window_h - movable->dst.GetH());
    const int base_x = std::clamp(movable->dst.GetX(), 0, max_x);
    const int base_y = std::clamp(movable->dst.GetY(), 0, max_y);

    for (auto& sprite : sprites) {
        if (!sprite.anchor_to_movable) {
            continue;
        }
        const int desired_x = base_x + sprite.anchor_offset_x;
        const int desired_y = base_y + sprite.anchor_offset_y;
        const int clamp_x_max = has_tilemap ? std::max(0, map_px_w - sprite.dst.GetW())
                                             : std::max(0, window_w - sprite.dst.GetW());
        const int clamp_y_max = has_tilemap ? std::max(0, map_px_h - sprite.dst.GetH())
                                             : std::max(0, window_h - sprite.dst.GetH());
        const int clamped_x = std::clamp(desired_x, 0, clamp_x_max);
        const int clamped_y = std::clamp(desired_y, 0, clamp_y_max);
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
