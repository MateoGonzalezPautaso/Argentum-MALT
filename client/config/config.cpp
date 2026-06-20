#include "config.h"

#include <stdexcept>
#include <utility>

#include <toml++/toml.h>

#include "../../common/item_catalog.h"

namespace {

std::vector<std::string> parse_paths(const toml::table& tbl) {
    std::vector<std::string> paths;

    if (auto path_value = tbl["path"].value<std::string>()) {
        paths.push_back(*path_value);
    }

    if (auto paths_array = tbl["paths"].as_array()) {
        for (const auto& item: *paths_array) {
            if (auto value = item.value<std::string>()) {
                paths.push_back(*value);
            }
        }
    }

    return paths;
}

void parse_network_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["network"].as_table()) {
        config.network.host = toml_get_string(*tbl, "host", config.network.host);
        config.network.port = toml_get_string(*tbl, "port", config.network.port);
    }
}

void parse_window_config(const toml::table& root, ClientConfig& config) {
    if (auto window = root["window"].as_table()) {
        config.window.width = toml_get_int(*window, "width", config.window.width);
        config.window.height = toml_get_int(*window, "height", config.window.height);
        config.window.title = toml_get_string(*window, "title", config.window.title);
        config.window.fullscreen = toml_get_bool(*window, "fullscreen", config.window.fullscreen);
    }
}

void parse_background_config(const toml::table& root, ClientConfig& config) {
    if (auto background = root["background"].as_table()) {
        config.background.path = toml_get_string(*background, "path", std::string());
        config.background.x = toml_get_int(*background, "x", config.background.x);
        config.background.y = toml_get_int(*background, "y", config.background.y);
        config.background.width = toml_get_int(*background, "width", config.window.width);
        config.background.height = toml_get_int(*background, "height", config.window.height);
    }

    if (config.background.path.empty()) {
        throw std::runtime_error("background.path is required in config");
    }
}

void parse_sprite_configs(const toml::table& root, ClientConfig& config) {
    if (auto sprites = root["sprites"].as_array()) {
        for (const auto& entry: *sprites) {
            if (!entry.is_table()) {
                continue;
            }
            const toml::table& sprite_tbl = *entry.as_table();
            SpriteConfig sprite;
            sprite.paths = parse_paths(sprite_tbl);
            sprite.x = toml_get_int(sprite_tbl, "x", sprite.x);
            sprite.y = toml_get_int(sprite_tbl, "y", sprite.y);
            sprite.width = toml_get_int(sprite_tbl, "width", sprite.width);
            sprite.height = toml_get_int(sprite_tbl, "height", sprite.height);
            sprite.src_x = toml_get_int(sprite_tbl, "src_x", sprite.src_x);
            sprite.src_y = toml_get_int(sprite_tbl, "src_y", sprite.src_y);
            sprite.src_width = toml_get_int(sprite_tbl, "src_width", sprite.src_width);
            sprite.src_height = toml_get_int(sprite_tbl, "src_height", sprite.src_height);
            sprite.frame_ms = toml_get_uint32(sprite_tbl, "frame_ms", sprite.frame_ms);
            sprite.movable = toml_get_bool(sprite_tbl, "movable", sprite.movable);
            sprite.anchor_to_movable =
                    toml_get_bool(sprite_tbl, "anchor_to_movable", sprite.anchor_to_movable);
            sprite.anchor_offset_x =
                    toml_get_int(sprite_tbl, "anchor_offset_x", sprite.anchor_offset_x);
            sprite.anchor_offset_y =
                    toml_get_int(sprite_tbl, "anchor_offset_y", sprite.anchor_offset_y);
            sprite.visible = toml_get_bool(sprite_tbl, "visible", sprite.visible);

            if (!sprite.paths.empty() && sprite.frame_ms == 0 && sprite.paths.size() > 1) {
                sprite.frame_ms = 120;
            }

            if (!sprite.paths.empty()) {
                config.sprites.push_back(sprite);
            }
        }
    }

    if (config.sprites.empty()) {
        throw std::runtime_error("config needs at least one sprite entry");
    }
}

void parse_font_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["font"].as_table()) {
        config.font.path = toml_get_string(*tbl, "path", config.font.path);
        config.font.name_size = toml_get_int(*tbl, "name_size", config.font.name_size);
        config.font.chat_size = toml_get_int(*tbl, "chat_size", config.font.chat_size);
        config.font.bar_size = toml_get_int(*tbl, "bar_size", config.font.bar_size);
        config.font.field_size = toml_get_int(*tbl, "field_size", config.font.field_size);
        config.font.title_size = toml_get_int(*tbl, "title_size", config.font.title_size);
        config.ui.font_path = config.font.path;
        config.ui.font_name_size = config.font.name_size;
        config.ui.font_chat_size = config.font.chat_size;
        config.ui.font_bar_size = config.font.bar_size;
        config.ui.font_field_size = config.font.field_size;
        config.ui.font_title_size = config.font.title_size;
    }
}

void parse_viewport_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["viewport"].as_table()) {
        config.viewport.logical_w = toml_get_int(*tbl, "logical_w", config.viewport.logical_w);
        config.viewport.logical_h = toml_get_int(*tbl, "logical_h", config.viewport.logical_h);
        config.viewport.game_x = toml_get_int(*tbl, "game_x", config.viewport.game_x);
        config.viewport.game_y = toml_get_int(*tbl, "game_y", config.viewport.game_y);
        config.viewport.game_w = toml_get_int(*tbl, "game_w", config.viewport.game_w);
        config.viewport.game_h = toml_get_int(*tbl, "game_h", config.viewport.game_h);
    }
}

void parse_merchant_button(const toml::table& tbl, const char* key, MerchantButtonConfig& btn) {
    if (auto subtbl = tbl[key].as_table()) {
        btn.x = toml_get_int(*subtbl, "x", btn.x);
        btn.y = toml_get_int(*subtbl, "y", btn.y);
        btn.w = toml_get_int(*subtbl, "w", btn.w);
        btn.h = toml_get_int(*subtbl, "h", btn.h);
    }
}

void parse_stat_bar(const toml::table& tbl, const char* key, StatBarConfig& bar) {
    if (auto subtbl = tbl[key].as_table()) {
        bar.x = toml_get_int(*subtbl, "x", bar.x);
        bar.y = toml_get_int(*subtbl, "y", bar.y);
        bar.w = toml_get_int(*subtbl, "w", bar.w);
        bar.h = toml_get_int(*subtbl, "h", bar.h);
        bar.label = toml_get_string(*subtbl, "label", bar.label);
    }
}

void parse_ui_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["ui"].as_table()) {
        config.ui.window_w = toml_get_int(*tbl, "window_w", config.ui.window_w);
        config.ui.window_h = toml_get_int(*tbl, "window_h", config.ui.window_h);
        config.ui.placeholder_username =
                toml_get_string(*tbl, "placeholder_username", config.ui.placeholder_username);
        config.ui.placeholder_password =
                toml_get_string(*tbl, "placeholder_password", config.ui.placeholder_password);
        config.ui.title_text = toml_get_string(*tbl, "title_text", config.ui.title_text);
        config.ui.login_field_w = toml_get_int(*tbl, "login_field_w", config.ui.login_field_w);
        config.ui.login_field_h = toml_get_int(*tbl, "login_field_h", config.ui.login_field_h);
        config.ui.login_logo_y = toml_get_int(*tbl, "login_logo_y", config.ui.login_logo_y);
        config.ui.chat_input_x = toml_get_int(*tbl, "chat_input_x", config.ui.chat_input_x);
        config.ui.chat_input_y = toml_get_int(*tbl, "chat_input_y", config.ui.chat_input_y);
        config.ui.chat_input_w = toml_get_int(*tbl, "chat_input_w", config.ui.chat_input_w);
        config.ui.chat_input_h = toml_get_int(*tbl, "chat_input_h", config.ui.chat_input_h);
        config.ui.chat_history_x = toml_get_int(*tbl, "chat_history_x", config.ui.chat_history_x);
        config.ui.chat_history_y = toml_get_int(*tbl, "chat_history_y", config.ui.chat_history_y);
        config.ui.chat_history_w = toml_get_int(*tbl, "chat_history_w", config.ui.chat_history_w);
        config.ui.chat_history_h = toml_get_int(*tbl, "chat_history_h", config.ui.chat_history_h);
        config.ui.start_x = toml_get_int(*tbl, "start_x", config.ui.start_x);
        config.ui.start_y = toml_get_int(*tbl, "start_y", config.ui.start_y);
        config.ui.start_w = toml_get_int(*tbl, "start_w", config.ui.start_w);
        config.ui.start_h = toml_get_int(*tbl, "start_h", config.ui.start_h);
        config.ui.settings_x = toml_get_int(*tbl, "settings_x", config.ui.settings_x);
        config.ui.settings_y = toml_get_int(*tbl, "settings_y", config.ui.settings_y);
        config.ui.settings_w = toml_get_int(*tbl, "settings_w", config.ui.settings_w);
        config.ui.settings_h = toml_get_int(*tbl, "settings_h", config.ui.settings_h);
        config.ui.login_title_spacing =
                toml_get_int(*tbl, "login_title_spacing", config.ui.login_title_spacing);
        config.ui.login_field_spacing =
                toml_get_int(*tbl, "login_field_spacing", config.ui.login_field_spacing);
        config.ui.login_field_gap =
                toml_get_int(*tbl, "login_field_gap", config.ui.login_field_gap);
        config.ui.login_button_spacing =
                toml_get_int(*tbl, "login_button_spacing", config.ui.login_button_spacing);
        config.ui.back_button_x = toml_get_int(*tbl, "back_button_x", config.ui.back_button_x);
        config.ui.back_button_y = toml_get_int(*tbl, "back_button_y", config.ui.back_button_y);
        config.ui.error_spacing = toml_get_int(*tbl, "error_spacing", config.ui.error_spacing);
        config.ui.cursor_blink_ms =
                toml_get_uint32(*tbl, "cursor_blink_ms", config.ui.cursor_blink_ms);
        config.ui.cursor_width = toml_get_int(*tbl, "cursor_width", config.ui.cursor_width);
        config.ui.cursor_padding = toml_get_int(*tbl, "cursor_padding", config.ui.cursor_padding);
        config.ui.cursor_height_shrink =
                toml_get_int(*tbl, "cursor_height_shrink", config.ui.cursor_height_shrink);
        config.ui.chat_line_spacing =
                toml_get_int(*tbl, "chat_line_spacing", config.ui.chat_line_spacing);

        if (auto inv = (*tbl)["inventory_panel"].as_table()) {
            config.ui.inventory_panel.x = toml_get_int(*inv, "x", config.ui.inventory_panel.x);
            config.ui.inventory_panel.y = toml_get_int(*inv, "y", config.ui.inventory_panel.y);
            config.ui.inventory_panel.cols =
                    toml_get_int(*inv, "cols", config.ui.inventory_panel.cols);
            config.ui.inventory_panel.equip_slots =
                    toml_get_int(*inv, "equip_slots", config.ui.inventory_panel.equip_slots);
            config.ui.inventory_panel.slot_w =
                    toml_get_int(*inv, "slot_w", config.ui.inventory_panel.slot_w);
            config.ui.inventory_panel.slot_h =
                    toml_get_int(*inv, "slot_h", config.ui.inventory_panel.slot_h);
            config.ui.inventory_panel.gap =
                    toml_get_int(*inv, "gap", config.ui.inventory_panel.gap);
            config.ui.inventory_panel.equip_y =
                    toml_get_int(*inv, "equip_y", config.ui.inventory_panel.equip_y);
            config.ui.inventory_panel.equip_weapon_label = toml_get_string(
                    *inv, "equip_weapon_label", config.ui.inventory_panel.equip_weapon_label);
            config.ui.inventory_panel.equip_armor_label = toml_get_string(
                    *inv, "equip_armor_label", config.ui.inventory_panel.equip_armor_label);
            config.ui.inventory_panel.equip_helmet_label = toml_get_string(
                    *inv, "equip_helmet_label", config.ui.inventory_panel.equip_helmet_label);
            config.ui.inventory_panel.equip_shield_label = toml_get_string(
                    *inv, "equip_shield_label", config.ui.inventory_panel.equip_shield_label);
        }
    }

    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{}, "hp_bar",
                   config.ui.hp_bar);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{}, "mp_bar",
                   config.ui.mp_bar);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{}, "exp_bar",
                   config.ui.exp_bar);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{}, "gold_rect",
                   config.ui.gold_rect);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "potion_hp_button", config.ui.potion_hp);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "potion_mana_button", config.ui.potion_mana);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "crit_rect", config.ui.crit_rect);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "dodge_rect", config.ui.dodge_rect);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "strength_rect", config.ui.strength_rect);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "agility_rect", config.ui.agility_rect);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "damage_rect", config.ui.damage_rect);
    parse_stat_bar(root["ui"].as_table() ? *root["ui"].as_table() : toml::table{},
                   "defense_rect", config.ui.defense_rect);

    if (auto tbl = root["ui"].as_table()) {
        if (auto merchant_tbl = (*tbl)["merchant"].as_table()) {
            auto& m = config.ui.merchant;
            m.panel_x = toml_get_int(*merchant_tbl, "panel_x", m.panel_x);
            m.panel_y = toml_get_int(*merchant_tbl, "panel_y", m.panel_y);
            m.panel_w = toml_get_int(*merchant_tbl, "panel_w", m.panel_w);
            m.panel_h = toml_get_int(*merchant_tbl, "panel_h", m.panel_h);
            parse_merchant_button(*merchant_tbl, "buy",  m.buy);
            parse_merchant_button(*merchant_tbl, "sell", m.sell);
            m.list_offset_x = toml_get_int(*merchant_tbl, "list_offset_x", m.list_offset_x);
            m.list_offset_y = toml_get_int(*merchant_tbl, "list_offset_y", m.list_offset_y);
            m.list_w        = toml_get_int(*merchant_tbl, "list_w",         m.list_w);
            m.list_h        = toml_get_int(*merchant_tbl, "list_h",         m.list_h);
            m.row_h         = toml_get_int(*merchant_tbl, "row_h",          m.row_h);
            m.price_offset_x      = toml_get_int(*merchant_tbl,    "price_offset_x",      m.price_offset_x);
            m.sell_price_offset_x = toml_get_int(*merchant_tbl,    "sell_price_offset_x", m.sell_price_offset_x);
            m.sell_price_ratio    = toml_get_double(*merchant_tbl, "sell_price_ratio",    m.sell_price_ratio);
        }
    }

    if (auto portrait = root["ui"].as_table()) {
        if (auto pt = (*portrait)["portrait"].as_table()) {
            config.ui.portrait.x = toml_get_int(*pt, "x", config.ui.portrait.x);
            config.ui.portrait.y = toml_get_int(*pt, "y", config.ui.portrait.y);
            config.ui.portrait.w = toml_get_int(*pt, "w", config.ui.portrait.w);
            config.ui.portrait.h = toml_get_int(*pt, "h", config.ui.portrait.h);
            config.ui.portrait.lvl_x = toml_get_int(*pt, "lvl_x", config.ui.portrait.lvl_x);
            config.ui.portrait.lvl_y = toml_get_int(*pt, "lvl_y", config.ui.portrait.lvl_y);
            config.ui.portrait.head_src_w =
                    toml_get_int(*pt, "head_src_w", config.ui.portrait.head_src_w);
            config.ui.portrait.head_src_h =
                    toml_get_int(*pt, "head_src_h", config.ui.portrait.head_src_h);
            config.ui.portrait.body_src_w =
                    toml_get_int(*pt, "body_src_w", config.ui.portrait.body_src_w);
            config.ui.portrait.body_shoulder_h =
                    toml_get_int(*pt, "body_shoulder_h", config.ui.portrait.body_shoulder_h);
            config.ui.portrait.anchor_y =
                    toml_get_int(*pt, "anchor_y", config.ui.portrait.anchor_y);
            config.ui.portrait.zoom =
                    static_cast<float>(toml_get_double(*pt, "zoom", config.ui.portrait.zoom));
        }
    }
}

void parse_assets_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["assets"].as_table()) {
        config.ui.asset_menu_bg = toml_get_string(*tbl, "menu_bg", config.ui.asset_menu_bg);
        config.ui.asset_start_default =
                toml_get_string(*tbl, "start_default", config.ui.asset_start_default);
        config.ui.asset_start_hover =
                toml_get_string(*tbl, "start_hover", config.ui.asset_start_hover);
        config.ui.asset_settings_default =
                toml_get_string(*tbl, "settings_default", config.ui.asset_settings_default);
        config.ui.asset_settings_hover =
                toml_get_string(*tbl, "settings_hover", config.ui.asset_settings_hover);
        config.ui.asset_login_bg = toml_get_string(*tbl, "login_bg", config.ui.asset_login_bg);
        config.ui.asset_login_logo =
                toml_get_string(*tbl, "login_logo", config.ui.asset_login_logo);
        config.ui.asset_connect_default =
                toml_get_string(*tbl, "connect_default", config.ui.asset_connect_default);
        config.ui.asset_connect_hover =
                toml_get_string(*tbl, "connect_hover", config.ui.asset_connect_hover);
        config.ui.asset_back_default =
                toml_get_string(*tbl, "back_default", config.ui.asset_back_default);
        config.ui.asset_back_hover =
                toml_get_string(*tbl, "back_hover", config.ui.asset_back_hover);
        config.ui.asset_ui_frame = toml_get_string(*tbl, "ui_frame", config.ui.asset_ui_frame);
        config.ui.asset_hp_bar = toml_get_string(*tbl, "hp_bar", config.ui.asset_hp_bar);
        config.ui.asset_mp_bar = toml_get_string(*tbl, "mp_bar", config.ui.asset_mp_bar);
        config.ui.asset_exp_bar = toml_get_string(*tbl, "exp_bar", config.ui.asset_exp_bar);
        config.ui.asset_merchant_bg =
                toml_get_string(*tbl, "merchant_bg", config.ui.asset_merchant_bg);
        config.ui.asset_buy_default =
                toml_get_string(*tbl, "buy_default", config.ui.asset_buy_default);
        config.ui.asset_buy_hover =
                toml_get_string(*tbl, "buy_hover", config.ui.asset_buy_hover);
        config.ui.asset_sell_default =
                toml_get_string(*tbl, "sell_default", config.ui.asset_sell_default);
        config.ui.asset_sell_hover =
                toml_get_string(*tbl, "sell_hover", config.ui.asset_sell_hover);
        config.ui.asset_sell_disabled =
                toml_get_string(*tbl, "sell_disabled", config.ui.asset_sell_disabled);
    }
}

void parse_skin_config(const toml::table& root, ClientConfig& config) {
    auto skins_tbl = root["skins"].as_table();
    if (!skins_tbl) {
        return;
    }

    auto body_tbl = (*skins_tbl)["body"].as_table();
    if (body_tbl) {
        for (const auto& [key, value]: *body_tbl) {
            if (auto path = value.value<std::string>()) {
                config.skins.body[std::string(key.str())] = *path;
            }
        }
    }

    auto head_tbl = (*skins_tbl)["head"].as_table();
    if (head_tbl) {
        for (const auto& [key, value]: *head_tbl) {
            if (auto path = value.value<std::string>()) {
                config.skins.head[std::string(key.str())] = *path;
            }
        }
    }
}

void parse_movement_config(const toml::table& root, ClientConfig& config) {
    if (auto movement = root["movement"].as_table()) {
        config.move_step = toml_get_int(*movement, "move_step", config.move_step);
        config.walk_src_step = toml_get_int(*movement, "walk_src_step", config.walk_src_step);
        config.walk_src_frames = toml_get_int(*movement, "walk_src_frames", config.walk_src_frames);
        config.walk_src_frames_down =
                toml_get_int(*movement, "walk_src_frames_down", config.walk_src_frames);
        config.walk_src_frames_up =
                toml_get_int(*movement, "walk_src_frames_up", config.walk_src_frames);
        config.walk_src_frames_left =
                toml_get_int(*movement, "walk_src_frames_left", config.walk_src_frames);
        config.walk_src_frames_right =
                toml_get_int(*movement, "walk_src_frames_right", config.walk_src_frames);
        config.walk_frame_ms = toml_get_uint32(*movement, "walk_frame_ms", config.walk_frame_ms);
        config.tick_ms = toml_get_uint32(*movement, "tick_ms", config.tick_ms);
        config.dir_src_y_down = toml_get_int(*movement, "dir_src_y_down", config.dir_src_y_down);
        config.dir_src_y_up = toml_get_int(*movement, "dir_src_y_up", config.dir_src_y_up);
        config.dir_src_y_left = toml_get_int(*movement, "dir_src_y_left", config.dir_src_y_left);
        config.dir_src_y_right = toml_get_int(*movement, "dir_src_y_right", config.dir_src_y_right);
        config.head_dir_src_y_down =
                toml_get_int(*movement, "head_dir_src_y_down", config.head_dir_src_y_down);
        config.head_dir_src_y_up =
                toml_get_int(*movement, "head_dir_src_y_up", config.head_dir_src_y_up);
        config.head_dir_src_y_left =
                toml_get_int(*movement, "head_dir_src_y_left", config.head_dir_src_y_left);
        config.head_dir_src_y_right =
                toml_get_int(*movement, "head_dir_src_y_right", config.head_dir_src_y_right);
    }
}

void parse_item_sprites_config(const toml::table& root, ClientConfig& config) {
    if (auto sprites = root["item_sprites"].as_array()) {
        for (const auto& entry: *sprites) {
            if (!entry.is_table())
                continue;
            const toml::table& tbl = *entry.as_table();
            ItemSpriteDef def;
            std::string type_str = toml_get_string(tbl, "item_type", "");
            def.item_type = parse_item_type(type_str);
            if (def.item_type == ItemType::NONE)
                continue;

            def.path = toml_get_string(tbl, "path", "");
            def.src_x = toml_get_int(tbl, "src_x", def.src_x);
            def.src_y = toml_get_int(tbl, "src_y", def.src_y);
            def.src_w = toml_get_int(tbl, "src_w", def.src_w);
            def.src_h = toml_get_int(tbl, "src_h", def.src_h);
            def.category = toml_get_string(tbl, "category", "");
            def.color_r = static_cast<uint8_t>(toml_get_int(tbl, "color_r", def.color_r));
            def.color_g = static_cast<uint8_t>(toml_get_int(tbl, "color_g", def.color_g));
            def.color_b = static_cast<uint8_t>(toml_get_int(tbl, "color_b", def.color_b));

            config.item_sprites[static_cast<uint8_t>(def.item_type)] = def;
        }
    }
}

void parse_equip_overlays_config(const toml::table& root, ClientConfig& config) {
    if (auto overlays = root["equip_overlays"].as_array()) {
        for (const auto& entry: *overlays) {
            if (!entry.is_table())
                continue;
            const toml::table& tbl = *entry.as_table();
            EquipOverlayDef def;
            std::string type_str = toml_get_string(tbl, "item_type", "");
            def.item_type = parse_item_type(type_str);
            if (def.item_type == ItemType::NONE)
                continue;

            def.path = toml_get_string(tbl, "path", "");
            if (def.path.empty())
                continue;

            def.offset_y = toml_get_int(tbl, "offset_y", def.offset_y);

            if (auto val = tbl["static"].value<bool>())
                def.static_frame = *val;

            config.equip_overlays[static_cast<uint8_t>(def.item_type)] = def;
        }
    }
}
void parse_ground_item_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["ground_items"].as_table()) {
        config.ground_item.display_size =
                toml_get_int(*tbl, "display_size", config.ground_item.display_size);
        config.ground_item.float_amplitude =
                toml_get_int(*tbl, "float_amplitude", config.ground_item.float_amplitude);
        config.ground_item.float_period_ms =
                toml_get_uint32(*tbl, "float_period_ms", config.ground_item.float_period_ms);
    }
}

void parse_sfx_config(const toml::table& root, ClientConfig& config) {
    if (auto tbl = root["sfx"].as_table()) {
        for (const auto& [key, value]: *tbl) {
            if (auto path = value.value<std::string>()) {
                config.sfx.sounds[std::string(key.str())] = *path;
            }
        }
    }
}
}  // namespace

ClientConfig load_client_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ClientConfig config;

    parse_network_config(root, config);
    parse_window_config(root, config);
    parse_background_config(root, config);
    parse_sprite_configs(root, config);
    parse_font_config(root, config);
    parse_viewport_config(root, config);
    parse_ui_config(root, config);
    parse_assets_config(root, config);
    parse_skin_config(root, config);
    parse_movement_config(root, config);
    parse_item_sprites_config(root, config);
    parse_equip_overlays_config(root, config);
    parse_sfx_config(root, config);
    parse_ground_item_config(root, config);

    config.tilemap_configs = load_all_map_configs("config/map_list.toml");

    auto main_it = config.tilemap_configs.find("city");
    if (main_it != config.tilemap_configs.end()) {
        config.tilemap = main_it->second;
    } else if (!config.tilemap_configs.empty()) {
        config.tilemap = config.tilemap_configs.begin()->second;
    }

    return config;
}
