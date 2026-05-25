#include "tile_palette.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

TilePalette::TilePalette(TilemapConfig& config,
                         const std::unordered_map<std::string, QPixmap>& atlases, QWidget* parent):
        QWidget(parent), config_(config), atlases_(atlases) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* title = new QLabel("Tiles  (right-click: toggle walkable)");
    title->setStyleSheet("font-weight: bold; font-size: 14px; padding: 4px;");
    layout->addWidget(title);

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scroll_content = new QWidget();
    auto* scroll_layout = new QVBoxLayout(scroll_content);
    scroll_layout->setContentsMargins(0, 0, 0, 0);
    scroll_layout->setSpacing(0);

    button_group_ = new QButtonGroup(this);
    button_group_->setExclusive(true);

    int tsz = config_.tile_size;
    int preview_size = 64;

    std::vector<std::string> sorted_names;
    sorted_names.reserve(config_.tiles.size());
    std::transform(config_.tiles.begin(), config_.tiles.end(), std::back_inserter(sorted_names),
                   [](const auto& kv) { return kv.first; });
    std::sort(sorted_names.begin(), sorted_names.end());

    auto section = make_section(
            "Tiles", sorted_names, preview_size,
            [this, tsz](const std::string& name) -> QPixmap {
                const auto& def = config_.tiles.at(name);
                std::string atlas_path = def.path.empty() ? config_.path : def.path;
                auto it = atlases_.find(atlas_path);
                if (it == atlases_.end() || it->second.isNull())
                    return {};
                return it->second.copy(QRect(def.x, def.y, tsz, tsz));
            },
            [this](const std::string& name) {
                selected_tile_ = name;
                emit tile_selected(name);
            },
            true);
    sections_.push_back(std::move(section));
    scroll_layout->addWidget(sections_.back().header->parentWidget());

    std::vector<std::string> sorted_props;
    sorted_props.reserve(config_.props.size());
    std::transform(config_.props.begin(), config_.props.end(), std::back_inserter(sorted_props),
                   [](const auto& kv) { return kv.first; });
    std::sort(sorted_props.begin(), sorted_props.end());

    if (!sorted_props.empty()) {
        auto props_section = make_section(
                "Props", sorted_props, preview_size,
                [this, tsz](const std::string& name) -> QPixmap {
                    const auto& def = config_.props.at(name);
                    if (def.paths.empty())
                        return {};
                    auto it = atlases_.find(def.paths[0]);
                    if (it == atlases_.end() || it->second.isNull())
                        return {};
                    return it->second.copy(QRect(def.src_x, def.src_y, def.src_w, def.src_h));
                },
                [this](const std::string& name) {
                    selected_tile_ = name;
                    emit prop_selected(name);
                },
                false);
        sections_.push_back(std::move(props_section));
        scroll_layout->addWidget(sections_.back().header->parentWidget());
    }

    scroll_layout->addStretch();

    scroll->setWidget(scroll_content);
    layout->addWidget(scroll);
}

TilePalette::SectionWidgets TilePalette::make_section(
        const std::string& title, const std::vector<std::string>& names, int preview_size,
        std::function<QPixmap(const std::string&)> get_thumbnail,
        std::function<void(const std::string&)> on_click, bool enable_walkable_menu) {

    QString base =
            QString::fromStdString(title) + QString(" (%1)").arg(static_cast<int>(names.size()));

    auto* container = new QWidget();
    auto* vlay = new QVBoxLayout(container);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    auto* header = new QToolButton();
    header->setText("▼ " + base);
    header->setCheckable(true);
    header->setChecked(true);
    header->setToolButtonStyle(Qt::ToolButtonTextOnly);
    header->setStyleSheet("QToolButton {"
                          "  background: #e0e0e0;"
                          "  border: 1px solid #ccc;"
                          "  border-radius: 4px;"
                          "  padding: 6px;"
                          "  font-weight: bold;"
                          "  text-align: left;"
                          "}"
                          "QToolButton:hover {"
                          "  background: #d0d0d0;"
                          "}"
                          "QToolButton:checked {"
                          "  border-bottom: 1px solid #aaa;"
                          "  border-radius: 4px 4px 0 0;"
                          "}");
    header->setFixedHeight(32);

    auto* content = new QWidget();
    auto* grid = new QGridLayout(content);
    grid->setSpacing(4);

    int col = 0, row = 0;
    for (const auto& name: names) {
        auto* btn = new QToolButton();
        btn->setText(QString::fromStdString(name));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setCheckable(true);
        btn->setFixedSize(90, 90);

        QPixmap thumb = get_thumbnail(name);
        if (!thumb.isNull()) {
            btn->setIcon(QIcon(thumb.scaled(preview_size, preview_size, Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation)));
            btn->setIconSize(QSize(preview_size, preview_size));
        }

        if (enable_walkable_menu) {
            const auto& def = config_.tiles.at(name);
            btn->setToolTip(
                    QString::fromStdString(name + (def.walkable ? " (walkable)" : " (blocked)")));
            if (!def.walkable) {
                btn->setStyleSheet("QToolButton { border: 2px solid red; }");
            }
            btn->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(btn, &QToolButton::customContextMenuRequested, this,
                    [this, name]() { toggle_walkable(name); });
        }

        button_group_->addButton(btn);

        connect(btn, &QToolButton::clicked, this, [this, name, on_click]() {
            selected_tile_ = name;
            on_click(name);
        });

        tile_buttons_[name] = btn;

        grid->addWidget(btn, row, col);
        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }

    vlay->addWidget(header);
    vlay->addWidget(content);

    connect(header, &QToolButton::toggled, this, [header, content, base](bool checked) {
        content->setVisible(checked);
        header->setText(QString(checked ? "▼ " : "▶ ") + base);
    });

    return {header, content};
}

void TilePalette::toggle_walkable(const std::string& name) {
    auto it = config_.tiles.find(name);
    if (it == config_.tiles.end())
        return;

    it->second.walkable = !it->second.walkable;

    auto btn_it = tile_buttons_.find(name);
    if (btn_it != tile_buttons_.end()) {
        update_button_visual(btn_it->second, name);
    }
}

void TilePalette::update_button_visual(QToolButton* btn, const std::string& name) const {
    auto it = config_.tiles.find(name);
    if (it == config_.tiles.end())
        return;

    const auto& def = it->second;
    btn->setToolTip(QString::fromStdString(name + (def.walkable ? " (walkable)" : " (blocked)")));
    btn->setStyleSheet(def.walkable ? "" : "QToolButton { border: 2px solid red; }");
}
