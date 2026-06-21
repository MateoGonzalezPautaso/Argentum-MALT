#ifndef CLIENT_GROUND_ITEM_RENDERER_H
#define CLIENT_GROUND_ITEM_RENDERER_H

#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include <SDL2pp/SDL2pp.hh>

#include "../../../common/messages.h"
#include "../../config/config.h"
#include "../texture_cache.h"

class GroundItemRenderer {
public:
    GroundItemRenderer(SDL2pp::Renderer& renderer,
                       const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites,
                       const GroundItemConfig& cfg = {});

    void apply_config(const GroundItemConfig& cfg) { config_ = cfg; }

    void add_item(int world_x, int world_y, ItemType type, const std::string& name);
    void remove_item(int world_x, int world_y, const std::string& item_name);
    void clear();
    void render(const SDL2pp::Rect& cam);

private:
    struct GroundItem {
        ItemType type;
        std::string name;
    };

    SDL2pp::Renderer& renderer;
    const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites;
    TextureCache texture_cache_;
    std::map<std::pair<int, int>, std::vector<GroundItem>> items;
    GroundItemConfig config_;

    void draw_item(const GroundItem& item, int screen_x, int screen_y);
};

#endif  // CLIENT_GROUND_ITEM_RENDERER_H
