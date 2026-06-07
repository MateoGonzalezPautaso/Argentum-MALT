#include "main_window.h"

#include <QActionGroup>
#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPen>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

#include "../io/file_manager.h"
#include "../render/map_scene_renderer.h"
#include "../ui/dialogs.h"
#include "../ui/tile_palette.h"

MainWindow::MainWindow(const std::string& config_path, QWidget* parent): QMainWindow(parent) {
    try {
        doc_.load(config_path);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Load Error",
                              QString("Failed to load %1:\n%2")
                                      .arg(QString::fromStdString(config_path), e.what()));
        QApplication::quit();
        return;
    }

    try {
        toml::table root = toml::parse_file("config/common_tilemap.toml");
        parse_tilemap_config(root, default_tile_config_);
        parse_prop_config(root, default_tile_config_);
    } catch (...) {
    }

    atlas_loader_.load(doc_.config());
    file_manager_ = new FileManager(this);
    setup_ui();
    renderer_ = new MapSceneRenderer(scene_, atlas_loader_);
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);
    renderer_->rebuild_spawn_overlay(doc_);

    update_title();
    resize(1200, 800);
}

void MainWindow::setup_ui() {
    auto* central = new QWidget(this);
    setCentralWidget(central);

    splitter_ = new QSplitter(Qt::Horizontal, central);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter_);

    auto* canvas_container = new QWidget();
    auto* canvas_layout = new QVBoxLayout(canvas_container);
    canvas_layout->setContentsMargins(0, 0, 0, 0);

    scene_ = new QGraphicsScene(this);
    view_ = new QGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing, false);
    view_->setDragMode(QGraphicsView::NoDrag);
    view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view_->viewport()->installEventFilter(this);
    canvas_layout->addWidget(view_);

    splitter_->addWidget(canvas_container);

    palette_ = new TilePalette(doc_.config(), atlas_loader_.all(), this);
    splitter_->addWidget(palette_);

    connect_palette_signals();

    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 1);

    statusBar()->showMessage(QString("Map: %1 x %2 tiles  |  Tile size: %3 px")
                                     .arg(doc_.width())
                                     .arg(doc_.height())
                                     .arg(doc_.tile_size()));

    auto* toolbar = addToolBar("Map");
    toolbar->setMovable(false);

    auto* width_label = new QLabel("Width:");
    width_spin_ = new QSpinBox();
    width_spin_->setRange(1, kMaxMapDimension);
    width_spin_->setValue(doc_.width());

    auto* height_label = new QLabel("  Height:");
    height_spin_ = new QSpinBox();
    height_spin_->setRange(1, kMaxMapDimension);
    height_spin_->setValue(doc_.height());

    auto* resize_btn = new QPushButton("Resize");
    toolbar->addWidget(width_label);
    toolbar->addWidget(width_spin_);
    toolbar->addWidget(height_label);
    toolbar->addWidget(height_spin_);
    toolbar->addWidget(resize_btn);

    connect(resize_btn, &QPushButton::clicked, this,
            [this]() { resize_map(width_spin_->value(), height_spin_->value()); });

    toolbar->addSeparator();
    spawn_zone_mode_action_ = toolbar->addAction("Set Spawn Zone");
    spawn_zone_mode_action_->setCheckable(true);
    connect(spawn_zone_mode_action_, &QAction::toggled, this, [this](bool checked) {
        spawn_zone_mode_ = checked;
        statusBar()->showMessage(checked ? QString("Spawn Zone Mode: click tiles to mark spawn zones")
                                         : QString("Spawn Zone Mode: off"),
                                 3000);
    });

    auto* file_menu = menuBar()->addMenu("&File");

    auto* new_action = file_menu->addAction("&New Map");
    new_action->setShortcut(QKeySequence::New);
    connect(new_action, &QAction::triggered, this, &MainWindow::new_map);

    auto* open_action = file_menu->addAction("&Open...");
    open_action->setShortcut(QKeySequence::Open);
    connect(open_action, &QAction::triggered, this, &MainWindow::open_map);

    file_menu->addSeparator();

    auto* save_action = file_menu->addAction("&Save");
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, &MainWindow::save_map);

    auto* save_as_action = file_menu->addAction("Save &As...");
    save_as_action->setShortcut(QKeySequence::SaveAs);
    connect(save_as_action, &QAction::triggered, this, &MainWindow::save_map_as);

    auto* map_menu = menuBar()->addMenu("&Map");
    auto* map_type_menu = map_menu->addMenu("Map &Type");
    map_type_group_ = new QActionGroup(this);

    auto add_type_action = [&](const QString& label, MapType type) {
        auto* action = map_type_menu->addAction(label);
        action->setCheckable(true);
        action->setData(static_cast<int>(type));
        action->setChecked(doc_.config().map_type == type);
        map_type_group_->addAction(action);
    };
    add_type_action("&None", MapType::NONE);
    add_type_action("&City", MapType::CITY);
    add_type_action("&Dungeon", MapType::DUNGEON);
    connect(map_type_group_, &QActionGroup::triggered, this, &MainWindow::change_map_type);

    auto* view_menu = menuBar()->addMenu("&View");
    auto* walkable_action = view_menu->addAction("Show &Walkable Overlay");
    walkable_action->setCheckable(true);
    walkable_action->setChecked(show_walkable_overlay_);
    connect(walkable_action, &QAction::triggered, this, &MainWindow::toggle_walkable_overlay);

    auto* spawn_overlay_action = view_menu->addAction("Show Spawn &Zone Overlay");
    spawn_overlay_action->setCheckable(true);
    spawn_overlay_action->setChecked(true);
    connect(spawn_overlay_action, &QAction::triggered, this, &MainWindow::toggle_spawn_overlay);
}

void MainWindow::connect_palette_signals() {
    connect(palette_, &TilePalette::tile_selected, this, [this](const std::string& name) {
        interaction_.set_selected(name);
        statusBar()->showMessage(QString("Selected tile: %1  |  Map: %2 x %3")
                                         .arg(QString::fromStdString(name))
                                         .arg(doc_.width())
                                         .arg(doc_.height()));
    });
    connect(palette_, &TilePalette::prop_selected, this, [this](const std::string& name) {
        interaction_.set_selected(name);
        statusBar()->showMessage(QString("Selected prop: %1  |  Map: %2 x %3")
                                         .arg(QString::fromStdString(name))
                                         .arg(doc_.width())
                                         .arg(doc_.height()));
    });
}

void MainWindow::full_rebuild() {
    renderer_->clear_all();
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);
    renderer_->rebuild_spawn_overlay(doc_);
    width_spin_->setValue(doc_.width());
    height_spin_->setValue(doc_.height());
    view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::rebuild_palette() {
    delete palette_;
    palette_ = new TilePalette(doc_.config(), atlas_loader_.all(), this);
    splitter_->addWidget(palette_);
    connect_palette_signals();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj != view_->viewport())
        return QMainWindow::eventFilter(obj, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        auto click = MapInteraction::resolve_click(me, view_, doc_.tile_size(), doc_.height(),
                                                   doc_.width());
        if (!click.valid)
            return false;

        if (spawn_zone_mode_) {
            if (click.button == Qt::LeftButton) {
                dragging_ = true;
                drag_start_row_ = click.row;
                drag_start_col_ = click.col;
                doc_.set_mob_spawn_zone(click.row, click.col, true);
                renderer_->update_spawn_overlay_tile(click.row, click.col, true, doc_.tile_size());
                return true;
            }
            if (click.button == Qt::RightButton) {
                doc_.set_mob_spawn_zone(click.row, click.col, false);
                renderer_->update_spawn_overlay_tile(click.row, click.col, false,
                                                     doc_.tile_size());
                return true;
            }
        }

        if (click.button == Qt::LeftButton && interaction_.has_selection()) {
            dragging_ = true;
            drag_start_row_ = click.row;
            drag_start_col_ = click.col;
            place_tile_or_prop(click.row, click.col, interaction_.selected());
            return true;
        }
        if (click.button == Qt::RightButton) {
            if (!doc_.prop_name(click.row, click.col).empty()) {
                doc_.set_prop(click.row, click.col, "");
                renderer_->update_prop(click.row, click.col, "", doc_);
            } else {
                doc_.set_tile(click.row, click.col, "");
                renderer_->update_tile(click.row, click.col, "", doc_, show_walkable_overlay_);
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove && dragging_) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPointF scene_pos = view_->mapToScene(me->pos());
        int tsz = doc_.tile_size();
        int cur_row = static_cast<int>(scene_pos.y()) / tsz;
        int cur_col = static_cast<int>(scene_pos.x()) / tsz;

        if (cur_row >= 0 && cur_row < doc_.height() && cur_col >= 0 && cur_col < doc_.width()) {
            update_drag_preview(drag_start_row_, drag_start_col_, cur_row, cur_col);
        }
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && dragging_) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton)
            return false;

        auto click = MapInteraction::resolve_click(me, view_, doc_.tile_size(), doc_.height(),
                                                   doc_.width());
        dragging_ = false;
        destroy_drag_preview();

        if (!click.valid) {
            drag_start_row_ = -1;
            drag_start_col_ = -1;
            return true;
        }

        if (spawn_zone_mode_) {
            if (click.row != drag_start_row_ || click.col != drag_start_col_) {
                fill_spawn_zone_rect(drag_start_row_, drag_start_col_, click.row, click.col);
            }
        } else if (interaction_.has_selection() &&
                   (click.row != drag_start_row_ || click.col != drag_start_col_)) {
            fill_rect(drag_start_row_, drag_start_col_, click.row, click.col,
                      interaction_.selected());
        }

        drag_start_row_ = -1;
        drag_start_col_ = -1;
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::place_tile_or_prop(int row, int col, const std::string& name) {
    if (doc_.is_prop(name)) {
        doc_.set_prop(row, col, name);
        renderer_->update_prop(row, col, name, doc_);
    } else {
        doc_.set_tile(row, col, name);
        renderer_->update_tile(row, col, name, doc_, show_walkable_overlay_);
    }
}

void MainWindow::fill_rect(int r1, int c1, int r2, int c2, const std::string& name) {
    int r_min = std::min(r1, r2);
    int r_max = std::max(r1, r2);
    int c_min = std::min(c1, c2);
    int c_max = std::max(c1, c2);

    for (int r = r_min; r <= r_max; ++r) {
        for (int c = c_min; c <= c_max; ++c) {
            place_tile_or_prop(r, c, name);
        }
    }
}

void MainWindow::update_drag_preview(int r1, int c1, int r2, int c2) {
    int tsz = doc_.tile_size();
    int r_min = std::min(r1, r2);
    int r_max = std::max(r1, r2);
    int c_min = std::min(c1, c2);
    int c_max = std::max(c1, c2);

    if (!drag_preview_) {
        drag_preview_ =
                scene_->addRect(0, 0, 0, 0, QPen(QColor(255, 255, 255, 180), 2, Qt::DashLine),
                                QBrush(QColor(100, 200, 255, 40)));
    }

    drag_preview_->setRect(c_min * tsz, r_min * tsz, (c_max - c_min + 1) * tsz,
                           (r_max - r_min + 1) * tsz);
}

void MainWindow::destroy_drag_preview() {
    if (drag_preview_) {
        scene_->removeItem(drag_preview_);
        delete drag_preview_;
        drag_preview_ = nullptr;
    }
}

bool MainWindow::validate_city_map() const {
    if (doc_.config().map_type != MapType::CITY) {
        return true;
    }

    bool has_merchant = false;
    bool has_banker = false;
    bool has_priest = false;

    for (const auto& row : doc_.config().prop_map) {
        for (const auto& cell : row) {
            if (cell == "comerciante") has_merchant = true;
            if (cell == "banquero")    has_banker = true;
            if (cell == "sacerdote")   has_priest = true;
        }
    }

    QStringList missing;
    if (!has_merchant) missing << "comerciante";
    if (!has_banker)   missing << "banquero";
    if (!has_priest)   missing << "sacerdote";

    if (missing.isEmpty()) {
        return true;
    }

    QMessageBox::critical(
            const_cast<MainWindow*>(this), "Cannot save City map",
            QString("A City map must contain the following NPCs:\n\n"
                    "  - %1\n\n"
                    "Please place them on the map before saving.")
                    .arg(missing.join("\n  - ")));
    return false;
}

void MainWindow::save_map() {
    if (!validate_city_map()) {
        return;
    }
    if (file_manager_->save(doc_)) {
        update_title();
        statusBar()->showMessage(QString("Saved to %1").arg(QString::fromStdString(doc_.path())),
                                 3000);
    }
}

void MainWindow::save_map_as() {
    if (!validate_city_map()) {
        return;
    }
    if (file_manager_->save_as(doc_)) {
        update_title();
        statusBar()->showMessage(QString("Saved to %1").arg(QString::fromStdString(doc_.path())),
                                 3000);
    }
}

void MainWindow::open_map() {
    if (!file_manager_->open(doc_))
        return;

    atlas_loader_.clear();
    atlas_loader_.load(doc_.config());

    rebuild_palette();
    full_rebuild();

    update_title();
    for (auto* action : map_type_group_->actions()) {
        action->setChecked(static_cast<MapType>(action->data().toInt()) == doc_.config().map_type);
    }
    statusBar()->showMessage(QString("Opened %1").arg(QString::fromStdString(doc_.path())), 3000);
}

void MainWindow::new_map() {
    auto result = show_new_map_dialog(this);
    if (!result.accepted)
        return;

    dragging_ = false;
    drag_preview_ = nullptr;
    renderer_->clear_all();
    doc_.create_new(result.height, result.width, default_tile_config_, result.map_type);
    atlas_loader_.clear();
    atlas_loader_.load(doc_.config());

    rebuild_palette();
    full_rebuild();

    update_title();
    for (auto* action : map_type_group_->actions()) {
        action->setChecked(static_cast<MapType>(action->data().toInt()) == doc_.config().map_type);
    }
    statusBar()->showMessage(
            QString("New map: %1 x %2 tiles").arg(result.width).arg(result.height));
}

void MainWindow::resize_map(int cols, int rows) {
    dragging_ = false;
    drag_preview_ = nullptr;
    renderer_->clear_all();
    doc_.resize(rows, cols, "");
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);
    renderer_->rebuild_spawn_overlay(doc_);

    width_spin_->setValue(doc_.width());
    height_spin_->setValue(doc_.height());
    view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);

    statusBar()->showMessage(
            QString("Map resized to %1 x %2 tiles").arg(doc_.width()).arg(doc_.height()));
}

void MainWindow::toggle_walkable_overlay() {
    show_walkable_overlay_ = !show_walkable_overlay_;
    renderer_->clear_tiles_and_props();
    renderer_->render_all(doc_, show_walkable_overlay_);
}

void MainWindow::update_title() {
    QString title = "Map Editor";
    if (!doc_.path().empty()) {
        title += " - " + QString::fromStdString(doc_.path());
    } else {
        title += " - Untitled";
    }
    switch (doc_.config().map_type) {
        case MapType::CITY:    title += " [City]";    break;
        case MapType::DUNGEON: title += " [Dungeon]"; break;
        default: break;
    }
    setWindowTitle(title);
}

void MainWindow::change_map_type(QAction* action) {
    auto type = static_cast<MapType>(action->data().toInt());
    doc_.config().map_type = type;
    update_title();
    statusBar()->showMessage(QString("Map type changed to %1")
                                     .arg(action->text().toLower()), 3000);
}

void MainWindow::fill_spawn_zone_rect(int r1, int c1, int r2, int c2) {
    int r_min = std::min(r1, r2);
    int r_max = std::max(r1, r2);
    int c_min = std::min(c1, c2);
    int c_max = std::max(c1, c2);

    for (int r = r_min; r <= r_max; ++r) {
        for (int c = c_min; c <= c_max; ++c) {
            doc_.set_mob_spawn_zone(r, c, true);
            renderer_->update_spawn_overlay_tile(r, c, true, doc_.tile_size());
        }
    }
}

void MainWindow::toggle_spawn_zone_mode() {
    spawn_zone_mode_ = !spawn_zone_mode_;
    statusBar()->showMessage(
            spawn_zone_mode_ ? QString("Spawn Zone Mode: click tiles to mark spawn zones")
                             : QString("Spawn Zone Mode: off"),
            3000);
}

void MainWindow::toggle_spawn_overlay() {
    bool show = !renderer_->show_spawn_overlay();
    renderer_->set_show_spawn_overlay(show);
    renderer_->rebuild_spawn_overlay(doc_);
    statusBar()->showMessage(show ? QString("Spawn zone overlay: visible")
                                  : QString("Spawn zone overlay: hidden"),
                             3000);
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (first_show_) {
        first_show_ = false;
        view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
}
