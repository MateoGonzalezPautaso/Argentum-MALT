#include "tile_palette.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

TilePalette::TilePalette(TilemapConfig& config,
                         std::unordered_map<std::string, QPixmap>& atlases,
                         QWidget* parent)
    : QWidget(parent), config_(config), atlases_(atlases) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* title = new QLabel("Tiles  (right-click: toggle walkable)");
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

    int tsz = config_.tile_size;
    int preview_size = 64;
    int col = 0, row = 0;

    std::vector<std::string> sorted_names;
    sorted_names.reserve(config_.tiles.size());
    for (const auto& kv : config_.tiles) {
        sorted_names.push_back(kv.first);
    }
    std::sort(sorted_names.begin(), sorted_names.end());

    for (const auto& name : sorted_names) {
        const auto& def = config_.tiles.at(name);
        auto* btn = new QToolButton();
        btn->setText(QString::fromStdString(name));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setCheckable(true);
        btn->setFixedSize(90, 90);

        btn->setToolTip(QString::fromStdString(
            name + (def.walkable ? " (walkable)" : " (blocked)")));

        if (!def.walkable) {
            btn->setStyleSheet("QToolButton { border: 2px solid red; }");
        }

        std::string atlas_path = def.path.empty() ? config_.path : def.path;
        auto atlas_it = atlases_.find(atlas_path);
        if (atlas_it != atlases_.end() && !atlas_it->second.isNull()) {
            QPixmap tile = atlas_it->second.copy(QRect(def.x, def.y, tsz, tsz));
            btn->setIcon(QIcon(tile.scaled(preview_size, preview_size,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation)));
            btn->setIconSize(QSize(preview_size, preview_size));
        }

        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QToolButton::customContextMenuRequested, this,
                [this, name]() {
                    toggle_walkable(name);
                });

        button_group_->addButton(btn);

        connect(btn, &QToolButton::clicked, this, [this, name]() {
            on_button_clicked(name);
        });

        tile_buttons_[name] = btn;

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

void TilePalette::toggle_walkable(const std::string& name) {
    auto it = config_.tiles.find(name);
    if (it == config_.tiles.end()) return;

    it->second.walkable = !it->second.walkable;

    auto btn_it = tile_buttons_.find(name);
    if (btn_it != tile_buttons_.end()) {
        update_button_visual(btn_it->second, name);
    }
}

void TilePalette::update_button_visual(QToolButton* btn, const std::string& name) const {
    auto it = config_.tiles.find(name);
    if (it == config_.tiles.end()) return;

    const auto& def = it->second;
    btn->setToolTip(QString::fromStdString(
        name + (def.walkable ? " (walkable)" : " (blocked)")));
    btn->setStyleSheet(def.walkable ? "" : "QToolButton { border: 2px solid red; }");
}
