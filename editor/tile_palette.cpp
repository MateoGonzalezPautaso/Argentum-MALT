#include "tile_palette.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

TilePalette::TilePalette(const TilemapConfig& config,
                         const QPixmap& atlas,
                         QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* title = new QLabel("Tiles");
    title->setStyleSheet("font-weight: bold; font-size: 14px; padding: 4px;");
    layout->addWidget(title);

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* content = new QWidget();
    auto* grid = new QGridLayout(content);
    grid->setSpacing(4);

    button_group_ = new QButtonGroup(this);
    button_group_->setExclusive(true);

    int tsz = config.tile_size;
    int preview_size = 64;
    int col = 0, row = 0;

    bool has_atlas = !atlas.isNull();

    for (const auto& [name, def] : config.tiles) {
        auto* btn = new QToolButton();
        btn->setText(QString::fromStdString(name));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setCheckable(true);
        btn->setFixedSize(90, 90);

        if (has_atlas) {
            QPixmap tile = atlas.copy(QRect(def.x, def.y, tsz, tsz));
            btn->setIcon(QIcon(tile.scaled(preview_size, preview_size,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation)));
            btn->setIconSize(QSize(preview_size, preview_size));
        }

        button_group_->addButton(btn);

        connect(btn, &QToolButton::clicked, this, [this, name]() {
            on_button_clicked(name);
        });

        grid->addWidget(btn, row, col);
        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }

    scroll->setWidget(content);
    layout->addWidget(scroll);
}

void TilePalette::on_button_clicked(const std::string& name) {
    selected_tile_ = name;
    emit tile_selected(name);
}
