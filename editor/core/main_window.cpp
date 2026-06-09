#include "main_window.h"

#include <QActionGroup>
#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPen>
#include <QWheelEvent>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "../input/map_interaction.h"
#include "../ui/dialogs.h"

#include "editor_controller.h"

MainWindow::MainWindow(const std::string& config_path, QWidget* parent): QMainWindow(parent) {
    scene_ = new QGraphicsScene(this);

    controller_ = std::make_unique<EditorController>(scene_);

    try {
        controller_->load_document(config_path);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Load Error",
                              QString("Failed to load %1:\n%2")
                                      .arg(QString::fromStdString(config_path), e.what()));
        QApplication::quit();
        return;
    }

    controller_->load_default_tile_config("config/common_tilemap.toml");
    controller_->reload_atlas();
    controller_->full_rebuild();

    setup_ui();
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

    view_ = new QGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing, false);
    view_->setDragMode(QGraphicsView::NoDrag);
    view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view_->viewport()->installEventFilter(this);
    canvas_layout->addWidget(view_);

    splitter_->addWidget(canvas_container);

    const auto& doc = controller_->document();
    palette_ = new TilePalette(doc.config(), controller_->atlas_loader().all(),
                               [this](const std::string& name, bool walkable) {
                                   controller_->set_tile_walkable(name, walkable);
                               }, this);
    splitter_->addWidget(palette_);

    connect_palette_signals();

    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 1);

    statusBar()->showMessage(QString("Map: %1 x %2 tiles  |  Tile size: %3 px")
                                     .arg(doc.width())
                                     .arg(doc.height())
                                     .arg(doc.tile_size()));

    zoom_label_ = new QLabel("Zoom: 100%");
    zoom_label_->setStyleSheet("padding: 0 8px;");
    statusBar()->addPermanentWidget(zoom_label_);

    auto* toolbar = addToolBar("Map");
    toolbar->setMovable(false);

    auto* width_label = new QLabel("Width:");
    width_spin_ = new QSpinBox();
    width_spin_->setRange(1, kMaxMapDimension);
    width_spin_->setValue(doc.width());

    auto* height_label = new QLabel("  Height:");
    height_spin_ = new QSpinBox();
    height_spin_->setRange(1, kMaxMapDimension);
    height_spin_->setValue(doc.height());

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
        action->setChecked(controller_->document().config().map_type == type);
        map_type_group_->addAction(action);
    };
    add_type_action("&None", MapType::NONE);
    add_type_action("&City", MapType::CITY);
    add_type_action("&Dungeon", MapType::DUNGEON);
    connect(map_type_group_, &QActionGroup::triggered, this, &MainWindow::change_map_type);

    auto* view_menu = menuBar()->addMenu("&View");
    auto* walkable_action = view_menu->addAction("Show &Walkable Overlay");
    walkable_action->setCheckable(true);
    walkable_action->setChecked(controller_->show_walkable_overlay());
    connect(walkable_action, &QAction::triggered, this, [this]() {
        controller_->toggle_walkable_overlay();
    });

    auto* spawn_overlay_action = view_menu->addAction("Show Spawn &Zone Overlay");
    spawn_overlay_action->setCheckable(true);
    spawn_overlay_action->setChecked(true);
    connect(spawn_overlay_action, &QAction::triggered, this, [this]() {
        controller_->toggle_spawn_overlay();
    });
}

void MainWindow::connect_palette_signals() {
    connect(palette_, &TilePalette::tile_selected, this, [this](const std::string& name) {
        interaction_.set_selected(name);
        const auto& doc = controller_->document();
        statusBar()->showMessage(QString("Selected tile: %1  |  Map: %2 x %3")
                                         .arg(QString::fromStdString(name))
                                         .arg(doc.width())
                                         .arg(doc.height()));
    });
    connect(palette_, &TilePalette::prop_selected, this, [this](const std::string& name) {
        interaction_.set_selected(name);
        const auto& doc = controller_->document();
        statusBar()->showMessage(QString("Selected prop: %1  |  Map: %2 x %3")
                                         .arg(QString::fromStdString(name))
                                         .arg(doc.width())
                                         .arg(doc.height()));
    });
    connect(palette_, &TilePalette::configure_portal, this, [this](const std::string& prop_name) {
        const auto& props = controller_->document().config().props;
        auto it = props.find(prop_name);
        if (it == props.end())
            return;

        const auto& def = it->second;
        auto result = show_transition_dialog(this, def.transition_map,
                                             def.transition_x, def.transition_y);
        if (!result.accepted)
            return;

        controller_->set_prop_transition(prop_name, result.transition_map,
                                         result.transition_x, result.transition_y);
        palette_->update_prop_visual(prop_name);
        statusBar()->showMessage(
            QString("Portal %1 -> %2 (spawn: %3, %4)")
                .arg(QString::fromStdString(prop_name),
                     QString::fromStdString(result.transition_map.empty()
                        ? "(none)" : result.transition_map))
                .arg(result.transition_x)
                .arg(result.transition_y), 5000);
    });
}

void MainWindow::rebuild_palette() {
    delete palette_;
    const auto& doc = controller_->document();
    palette_ = new TilePalette(doc.config(), controller_->atlas_loader().all(),
                               [this](const std::string& name, bool walkable) {
                                   controller_->set_tile_walkable(name, walkable);
                               }, this);
    splitter_->addWidget(palette_);
    connect_palette_signals();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj != view_->viewport())
        return QMainWindow::eventFilter(obj, event);

    auto& doc = controller_->document();

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        auto click = MapInteraction::resolve_click(me, view_, doc.tile_size(), doc.height(),
                                                   doc.width());
        if (!click.valid)
            return false;

        if (spawn_zone_mode_) {
            if (click.button == Qt::LeftButton) {
                dragging_ = true;
                drag_start_row_ = click.row;
                drag_start_col_ = click.col;
                controller_->set_spawn_zone(click.row, click.col, true);
                return true;
            }
            if (click.button == Qt::RightButton) {
                controller_->set_spawn_zone(click.row, click.col, false);
                return true;
            }
        }

        if (click.button == Qt::LeftButton && interaction_.has_selection()) {
            dragging_ = true;
            drag_start_row_ = click.row;
            drag_start_col_ = click.col;
            controller_->place_tile_or_prop(click.row, click.col, interaction_.selected());
            return true;
        }
        if (click.button == Qt::RightButton) {
            std::string prop = doc.prop_name(click.row, click.col);
            if (!prop.empty()) {
                QMenu menu;
                QAction* erase_action = menu.addAction("Erase");
                QAction* portal_action = menu.addAction("Configure portal instance...");
                QAction* chosen = menu.exec(view_->viewport()->mapToGlobal(me->pos()));
                if (chosen == erase_action) {
                    controller_->erase_prop(click.row, click.col);
                } else if (chosen == portal_action) {
                    const auto& props = controller_->document().config().props;
                    auto it = props.find(prop);
                    if (it == props.end())
                        return true;

                    const auto& base = it->second;
                    PropTransitionOverride current = doc.transition_override(click.row, click.col);
                    std::string cur_map = current.transition_map.empty() ?
                            base.transition_map : current.transition_map;
                    int cur_x = current.transition_map.empty() ?
                            base.transition_x : current.transition_x;
                    int cur_y = current.transition_map.empty() ?
                            base.transition_y : current.transition_y;

                    auto result = show_transition_dialog(this, cur_map, cur_x, cur_y);
                    if (!result.accepted)
                        return true;

                    controller_->set_prop_transition_override(
                            click.row, click.col,
                            result.transition_map, result.transition_x, result.transition_y);
                    controller_->full_rebuild();
                    statusBar()->showMessage(
                        QString("Portal instance %1 -> %2 (spawn: %3, %4)")
                            .arg(QString::fromStdString(prop),
                                 QString::fromStdString(result.transition_map.empty()
                                    ? "(none)" : result.transition_map))
                            .arg(result.transition_x)
                            .arg(result.transition_y), 5000);
                }
            } else {
                controller_->erase_tile(click.row, click.col);
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove && dragging_) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPointF scene_pos = view_->mapToScene(me->pos());
        int tsz = doc.tile_size();
        int cur_row = static_cast<int>(scene_pos.y()) / tsz;
        int cur_col = static_cast<int>(scene_pos.x()) / tsz;

        if (cur_row >= 0 && cur_row < doc.height() && cur_col >= 0 && cur_col < doc.width()) {
            update_drag_preview(drag_start_row_, drag_start_col_, cur_row, cur_col);
        }
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && dragging_) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton)
            return false;

        auto click = MapInteraction::resolve_click(me, view_, doc.tile_size(), doc.height(),
                                                   doc.width());
        dragging_ = false;
        destroy_drag_preview();

        if (!click.valid) {
            drag_start_row_ = -1;
            drag_start_col_ = -1;
            return true;
        }

        if (spawn_zone_mode_) {
            if (click.row != drag_start_row_ || click.col != drag_start_col_) {
                controller_->fill_spawn_zone_rect(drag_start_row_, drag_start_col_, click.row,
                                                  click.col);
            }
        } else if (interaction_.has_selection() &&
                   (click.row != drag_start_row_ || click.col != drag_start_col_)) {
            controller_->fill_rect(drag_start_row_, drag_start_col_, click.row, click.col,
                                   interaction_.selected());
        }

        drag_start_row_ = -1;
        drag_start_col_ = -1;
        return true;
    }

    if (event->type() == QEvent::Wheel) {
        auto* we = static_cast<QWheelEvent*>(event);
        if (!(we->modifiers() & Qt::ControlModifier))
            return false;

        double factor = we->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        zoom_level_ *= factor;
        view_->scale(factor, factor);
        zoom_label_->setText(QString("Zoom: %1%").arg(static_cast<int>(zoom_level_ * 100)));
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::save_map() {
    if (!controller_->save())
        return;
    update_title();
    statusBar()->showMessage(
            QString("Saved to %1").arg(QString::fromStdString(controller_->document().path())),
            3000);
}

void MainWindow::save_map_as() {
    if (!controller_->save_as())
        return;
    update_title();
    statusBar()->showMessage(
            QString("Saved to %1").arg(QString::fromStdString(controller_->document().path())),
            3000);
}

void MainWindow::open_map() {
    if (!controller_->open())
        return;

    rebuild_palette();
    controller_->full_rebuild();

    update_title();
    const auto& doc = controller_->document();
    for (auto* action : map_type_group_->actions()) {
        action->setChecked(static_cast<MapType>(action->data().toInt()) == doc.config().map_type);
    }
    statusBar()->showMessage(QString("Opened %1").arg(QString::fromStdString(doc.path())), 3000);
}

void MainWindow::new_map() {
    auto result = show_new_map_dialog(this);
    if (!result.accepted)
        return;

    dragging_ = false;
    drag_preview_ = nullptr;
    controller_->create_new_map(result.height, result.width, result.map_type);

    rebuild_palette();

    const auto& doc = controller_->document();
    width_spin_->setValue(doc.width());
    height_spin_->setValue(doc.height());
    view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);

    update_title();
    for (auto* action : map_type_group_->actions()) {
        action->setChecked(static_cast<MapType>(action->data().toInt()) == doc.config().map_type);
    }
    statusBar()->showMessage(
            QString("New map: %1 x %2 tiles").arg(result.width).arg(result.height));
}

void MainWindow::resize_map(int cols, int rows) {
    dragging_ = false;
    drag_preview_ = nullptr;
    controller_->resize_map(cols, rows);

    const auto& doc = controller_->document();
    width_spin_->setValue(doc.width());
    height_spin_->setValue(doc.height());
    view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);

    statusBar()->showMessage(
            QString("Map resized to %1 x %2 tiles").arg(doc.width()).arg(doc.height()));
}

void MainWindow::update_title() {
    const auto& doc = controller_->document();
    QString title = "Map Editor";
    if (!doc.path().empty()) {
        title += " - " + QString::fromStdString(doc.path());
    } else {
        title += " - Untitled";
    }
    switch (doc.config().map_type) {
        case MapType::CITY:    title += " [City]";    break;
        case MapType::DUNGEON: title += " [Dungeon]"; break;
        default: break;
    }
    setWindowTitle(title);
}

void MainWindow::change_map_type(QAction* action) {
    auto type = static_cast<MapType>(action->data().toInt());
    controller_->set_map_type(type);
    update_title();
    statusBar()->showMessage(QString("Map type changed to %1")
                                     .arg(action->text().toLower()), 3000);
}

void MainWindow::update_drag_preview(int r1, int c1, int r2, int c2) {
    int tsz = controller_->document().tile_size();
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

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (first_show_) {
        first_show_ = false;
        view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
}
