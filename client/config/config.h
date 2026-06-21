#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../common/config.h"
#include "../../common/messages.h"

struct NetworkConfig {
    std::string host = "127.0.0.1";
    std::string port = "1234";
};

struct WindowConfig {
    int width = 960;
    int height = 540;
    std::string title = "Argentum Online";
    bool fullscreen = false;
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
    bool anchor_to_movable = false;
    int anchor_offset_x = 0;
    int anchor_offset_y = 0;
    bool visible = true;
};

struct FontConfig {
    std::string path = "assets/OUTPUT/Cardo.ttf";
    int name_size = 12;
    int chat_size = 16;
    int bar_size = 11;
    int field_size = 18;
    int title_size = 28;
};

struct ViewportConfig {
    int logical_w = 1024;
    int logical_h = 768;
    int game_x = 11;
    int game_y = 149;
    int game_w = 734;
    int game_h = 608;
};

struct StatBarConfig {
    int x = 790;
    int y = 601;
    int w = 218;
    int h = 17;
    std::string label{};
};

struct PortraitConfig {
    int x = 643;
    int y = 46;
    int w = 88;
    int h = 85;
    int lvl_x = 731;
    int lvl_y = 131;
    int head_src_w = 27;
    int head_src_h = 40;
    int body_src_w = 27;
    int body_shoulder_h = 15;
    int anchor_y = -20;
    float zoom = 1.9f;
};

struct MerchantButtonConfig {
    int x = 0, y = 0, w = 0, h = 0;
};

struct MerchantUIConfig {
    int panel_x = 200, panel_y = 100, panel_w = 624, panel_h = 568;
    MerchantButtonConfig buy  {265, 622, 122, 28};
    MerchantButtonConfig sell {452, 622, 122, 28};
    int list_offset_x = 34;
    int list_offset_y = 114;
    int list_w = 474;
    int list_h = 283;
    int row_h = 20;
    int price_offset_x = 335;
    int sell_price_offset_x = 460;
    double sell_price_ratio = 0.5;
};

struct InventoryPanelConfig {
    int x = 782, y = 202;
    int cols = 4;
    int equip_slots = 20;
    int slot_w = 50, slot_h = 50;
    int gap = 4;
    int equip_y = 50;
    std::string equip_weapon_label{"Arma"};
    std::string equip_armor_label{"Armadura"};
    std::string equip_helmet_label{"Casco"};
    std::string equip_shield_label{"Escudo"};
};

struct ItemSpriteDef {
    ItemType item_type = ItemType::NONE;
    std::string path;
    int src_x = 0, src_y = 0;
    int src_w = 32, src_h = 32;
    std::string category;
    uint8_t color_r = 80;
    uint8_t color_g = 80;
    uint8_t color_b = 80;
};

struct UIConfig {
    int window_w = 1024;
    int window_h = 768;
    std::string placeholder_username = "Username";
    std::string placeholder_password = "Password";
    std::string title_text = "Sign in";
    int login_field_w = 320;
    int login_field_h = 34;
    int login_logo_y = 80;
    StatBarConfig hp_bar;
    StatBarConfig mp_bar{790, 629, 217, 17};
    StatBarConfig exp_bar{790, 657, 217, 17};
    StatBarConfig gold_rect{783, 557, 63, 20};
    StatBarConfig potion_hp{851, 557, 77, 20};
    StatBarConfig potion_mana{932, 557, 77, 20};
    StatBarConfig crit_rect{957, 698, 50, 22};
    StatBarConfig dodge_rect{957, 728, 50, 22};
    StatBarConfig strength_rect{876, 698, 50, 22};
    StatBarConfig agility_rect{876, 728, 50, 22};
    StatBarConfig damage_rect{793, 698, 50, 22};
    StatBarConfig defense_rect{793, 728, 50, 22};
    PortraitConfig portrait;
    int chat_input_x = 41;
    int chat_input_y = 122;
    int chat_input_w = 565;
    int chat_input_h = 20;
    int chat_history_x = 15;
    int chat_history_y = 35;
    int chat_history_w = 614;
    int chat_history_h = 81;
    int start_x = 347;
    int start_y = 568;
    int start_w = 330;
    int start_h = 65;
    int audio_x = 685;
    int audio_y = 335;
    int audio_w = 272;
    int audio_h = 43;
    int game_audio_x = 760;
    int game_audio_y = 3;
    int game_audio_w = 164;
    int game_audio_h = 22;
    std::string font_path = "assets/OUTPUT/Cardo.ttf";
    int font_name_size = 12;
    int font_chat_size = 16;
    int font_bar_size = 11;
    int font_field_size = 18;
    int font_title_size = 28;
    std::string asset_menu_bg = "assets/interface/en_ventanalauncher.bmp";
    std::string asset_start_default = "assets/interface/en_boton-comenzar-default.bmp";
    std::string asset_start_hover = "assets/interface/en_boton-comenzar-over.bmp";
    std::string asset_audio_default = "assets/interface/en_boton-audio-default.bmp";
    std::string asset_audio_hover = "assets/interface/en_boton-audio-over.bmp";
    std::string asset_audio_off = "assets/interface/en_boton-audio-off.bmp";
    std::string asset_login_bg = "assets/BabelUI/static/media/leather_brown..png";
    std::string asset_login_logo = "assets/BabelUI/static/media/ao20_logo_med..png";
    std::string asset_connect_default = "assets/interface/en_boton-conectar-default.bmp";
    std::string asset_connect_hover = "assets/interface/en_boton-conectar-over.bmp";
    std::string asset_back_default = "assets/interface/en_boton-volver-default.bmp";
    std::string asset_back_hover = "assets/interface/en_boton-volver-over.bmp";
    std::string asset_new_account_default = "assets/interface/en_boton-crear-pj-default.bmp";
    std::string asset_new_account_hover = "assets/interface/en_boton-crear-pj-over.bmp";
    std::string asset_ui_frame = "assets/interface/en_ventanaprincipal_edit.bmp";
    std::string asset_hp_bar = "assets/interface/en_barradevida.bmp";
    std::string asset_mp_bar = "assets/interface/en_barrademana.bmp";
    std::string asset_exp_bar = "assets/interface/en_barraexperiencia.bmp";
    std::string asset_merchant_bg   = "assets/interface/en_comerciar.bmp";
    std::string asset_buy_default   = "assets/interface/en_boton-comprar-default.bmp";
    std::string asset_buy_hover     = "assets/interface/en_boton-comprar-over.bmp";
    std::string asset_sell_default  = "assets/interface/en_boton-vender-default.bmp";
    std::string asset_sell_hover    = "assets/interface/en_boton-vender-over.bmp";
    std::string asset_sell_disabled = "assets/interface/en_boton-vender-off.bmp";
    std::string asset_expand_default = "assets/interface/en_boton-sm-mas-default.bmp";
    std::string asset_expand_hover   = "assets/interface/en_boton-sm-mas-over.bmp";
    std::string asset_expand_off     = "assets/interface/en_boton-sm-mas-off.bmp";
    std::string asset_big_chat       = "assets/interface/big_chat.bmp";


    int login_title_spacing = 20;
    int login_field_spacing = 30;
    int login_field_gap = 16;
    int login_button_spacing = 30;
    int back_button_x = 10;
    int back_button_y = 10;
    int error_spacing = 10;
    uint32_t cursor_blink_ms = 500;
    int cursor_width = 2;
    int cursor_padding = 4;
    int cursor_height_shrink = 8;
    int chat_line_spacing = 2;
    int chat_expand_x = 0;
    int chat_expand_btn_gap = 2;
    int chat_expand_btn_w = 21;
    int chat_expand_btn_h = 21;
    int chat_expand_amount = 200;
    InventoryPanelConfig inventory_panel;
    MerchantUIConfig merchant;
};

struct NpcSkinDef {
    std::string path;
    int frame_w = 0;
    int frame_h = 0;
    int src_x = 0;
    int src_y = 0;
    int frames_per_dir = 4;
    int walk_row_offset = 4;
    bool swap_lr = false;
    std::vector<int> row_positions;
    std::vector<int> frame_positions;
};

struct SkinConfig {
    std::map<std::string, std::string> body;
    std::map<std::string, std::string> head;
    std::map<uint16_t, NpcSkinDef> npc;

    std::string head_path_for(Race race) const {
        auto it = head.find(race_to_skin_key(race));
        return it != head.end() ? it->second : "";
    }

    std::string body_path_for(PlayerClass pc) const {
        auto it = body.find(class_to_skin_key(pc));
        return it != body.end() ? it->second : "";
    }

    std::string npc_path_for(uint16_t sprite_id) const {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.path : "";
    }
    int npc_frame_w(uint16_t sprite_id) const {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.frame_w : 0;
    }
    int npc_frame_h(uint16_t sprite_id) const {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.frame_h : 0;
    }
    int npc_src_x(uint16_t sprite_id) const {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.src_x : 0;
    }
    int npc_src_y(uint16_t sprite_id) const {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.src_y : 0;
    }
    int npc_frames_per_dir(uint16_t sprite_id) {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.frames_per_dir : 0;
    }
    int npc_walk_row_offset(uint16_t sprite_id) {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.walk_row_offset : 0;
    }
    bool npc_swap_lr(uint16_t sprite_id) {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.swap_lr : false;
    }
    const std::vector<int>& npc_row_positions(uint16_t sprite_id) {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.row_positions : empty_vec;
    }
    const std::vector<int>& npc_frame_positions(uint16_t sprite_id) {
        auto it = npc.find(sprite_id);
        return it != npc.end() ? it->second.frame_positions : empty_vec;
    }

private:
    std::vector<int> empty_vec;
    static std::string class_to_skin_key(PlayerClass pc) {
        switch (pc) {
            case PlayerClass::MAGE:
                return "mage";
            case PlayerClass::CLERIC:
                return "cleric";
            case PlayerClass::PALADIN:
                return "paladin";
            case PlayerClass::WARRIOR:
                return "warrior";
        }
        return "";
    }

    static std::string race_to_skin_key(Race race) {
        switch (race) {
            case Race::HUMAN:
                return "human";
            case Race::ELF:
                return "elf";
            case Race::DWARF:
                return "dwarf";
            case Race::GNOME:
                return "gnome";
        }
        return "";
    }
};

struct EquipOverlayDef {
    ItemType item_type = ItemType::NONE;
    std::string path;
    int offset_y = 0;
    bool static_frame = false;
};

struct SfxConfig {
    std::unordered_map<std::string, std::string> sounds;
};

struct GroundItemConfig {
    int display_size = 32;
    int float_amplitude = 4;
    uint32_t float_period_ms = 1200;
};

struct ClientConfig {
    NetworkConfig network;
    WindowConfig window;
    BackgroundConfig background;
    TilemapConfig tilemap;
    std::unordered_map<std::string, TilemapConfig> tilemap_configs;
    std::vector<SpriteConfig> sprites;
    SkinConfig skins;
    FontConfig font;
    ViewportConfig viewport;
    UIConfig ui;
    std::unordered_map<uint8_t, ItemSpriteDef> item_sprites;
    std::unordered_map<uint8_t, EquipOverlayDef> equip_overlays;
    int move_step = 4;
    int walk_src_step = 30;
    int walk_src_frames = 6;
    int walk_src_frames_down = 6;
    int walk_src_frames_up = 6;
    int walk_src_frames_left = 6;
    int walk_src_frames_right = 6;
    uint32_t walk_frame_ms = 120;
    uint32_t tick_ms = 33;
    int dir_src_y_down = 0;
    int dir_src_y_up = 40;
    int dir_src_y_left = 80;
    int dir_src_y_right = 120;
    int head_dir_src_y_down = 0;
    int head_dir_src_y_up = 64;
    int head_dir_src_y_left = 128;
    int head_dir_src_y_right = 192;
    SfxConfig sfx;
    GroundItemConfig ground_item;
};

ClientConfig load_client_config(const std::string& path);

#endif
