#ifndef EDITOR_TILE_PALETTE_H
#define EDITOR_TILE_PALETTE_H

#include <QPixmap>
#include <QToolButton>
#include <QWidget>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/config.h"

class QButtonGroup;

class TilePalette: public QWidget {
    Q_OBJECT

public:
    TilePalette(const TilemapConfig& config,
                const std::unordered_map<std::string, QPixmap>& atlases,
                std::function<void(const std::string&, bool)> on_toggle_walkable,
                QWidget* parent = nullptr);

    std::string selected_tile() const { return selected_tile_; }

signals:
    void tile_selected(const std::string& tile_name);
    void prop_selected(const std::string& prop_name);

private:
    struct SectionWidgets {
        QToolButton* header;
        QWidget* content;
    };

    SectionWidgets make_section(const std::string& title, const std::vector<std::string>& names,
                                int preview_size,
                                std::function<QPixmap(const std::string& name)> get_thumbnail,
                                std::function<void(const std::string& name)> on_click,
                                bool enable_walkable_menu);
    void toggle_walkable(const std::string& name);
    void update_button_visual(QToolButton* btn, const std::string& name, bool walkable) const;

    std::string selected_tile_;
    QButtonGroup* button_group_ = nullptr;
    const TilemapConfig& config_;
    const std::unordered_map<std::string, QPixmap>& atlases_;
    std::function<void(const std::string&, bool)> on_toggle_walkable_;
    std::unordered_map<std::string, QToolButton*> tile_buttons_;
    std::vector<SectionWidgets> sections_;
};

#endif
