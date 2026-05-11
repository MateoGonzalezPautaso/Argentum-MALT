#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

struct WindowConfig {
    int width = 960;
    int height = 540;
    std::string title = "Argentum Online";
};

struct BackgroundConfig {
    std::string path;
    int x = 0;
    int y = 0;
    int width = 960;
    int height = 540;
};

struct SpriteConfig {
    std::vector<std::string> paths;
    int x = 0;
    int y = 0;
    int width = 60;
    int height = 160;
    int src_x = 0;
    int src_y = 0;
    int src_width = 0;
    int src_height = 0;
    uint32_t frame_ms = 0;
    bool movable = false;
    bool visible = true;
};

struct ClientConfig {
    WindowConfig window;
    BackgroundConfig background;
    std::vector<SpriteConfig> sprites;
    int move_step = 4;
    int walk_src_step = 30;
    int walk_src_max = 60;
    uint32_t walk_frame_ms = 120;
    uint32_t tick_ms = 33;
    int dir_src_y_down = 0;
    int dir_src_y_up = 40;
    int dir_src_y_left = 80;
    int dir_src_y_right = 120;
};

ClientConfig load_client_config(const std::string& path);

#endif
