#ifndef EDITOR_TILE_PALETTE_H
#define EDITOR_TILE_PALETTE_H

#include <QWidget>
#include <QPixmap>
#include <string>
#include "../common/config.h"

class QButtonGroup;

class TilePalette : public QWidget {
    Q_OBJECT

public:
    TilePalette(const TilemapConfig& config,
                const QPixmap& atlas,
                QWidget* parent = nullptr);

    std::string selected_tile() const { return selected_tile_; }

signals:
    void tile_selected(const std::string& tile_name);

private:
    void on_button_clicked(const std::string& name);

    std::string selected_tile_;
    QButtonGroup* button_group_ = nullptr;
};

#endif
