#ifndef EDITOR_TILE_PALETTE_H
#define EDITOR_TILE_PALETTE_H

#include <QWidget>
#include <QPixmap>
#include <QToolButton>
#include <string>
#include <unordered_map>
#include "../common/config.h"

class QButtonGroup;

class TilePalette : public QWidget {
    Q_OBJECT

public:
    TilePalette(TilemapConfig& config,
                std::unordered_map<std::string, QPixmap>& atlases,
                QWidget* parent = nullptr);

    std::string selected_tile() const { return selected_tile_; }

signals:
    void tile_selected(const std::string& tile_name);

private:
    void on_button_clicked(const std::string& name);
    void toggle_walkable(const std::string& name);
    void update_button_visual(QToolButton* btn, const std::string& name) const;

    std::string selected_tile_;
    QButtonGroup* button_group_ = nullptr;
    TilemapConfig& config_;
    std::unordered_map<std::string, QPixmap>& atlases_;
    std::unordered_map<std::string, QToolButton*> tile_buttons_;
};

#endif
