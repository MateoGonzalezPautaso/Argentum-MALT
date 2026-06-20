#ifndef CLIENT_GROUND_ITEM_RENDERER_H
#define CLIENT_GROUND_ITEM_RENDERER_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <SDL2pp/SDL2pp.hh>

#include "../../../common/messages.h"
#include "../../config/config.h"

class GroundItemRenderer {
public:
    GroundItemRenderer(SDL2pp::Renderer& renderer,
                       const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites,
                       int display_size = 32);

    void add_item(int world_x, int world_y, ItemType type, const std::string& name);
    void remove_item(int world_x, int world_y);
    void clear();
    void render(const SDL2pp::Rect& cam);

private:
    struct GroundItem {
        ItemType type;
        std::string name;
    };

    SDL2pp::Renderer& renderer;
    const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites;
    std::unordered_map<std::string, std::unique_ptr<SDL2pp::Texture>> texture_cache;
    std::map<std::pair<int, int>, GroundItem> items;
    int display_size;

    SDL2pp::Texture* get_or_load_texture(const ItemSpriteDef& def);
    void draw_item(const GroundItem& item, int screen_x, int screen_y);
};

#endif  // CLIENT_GROUND_ITEM_RENDERER_H
