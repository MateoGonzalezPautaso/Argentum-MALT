#include "main_window.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

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

    atlas_loader_.load(doc_.config());
    file_manager_ = new FileManager(this);
    setup_ui();
    renderer_ = new MapSceneRenderer(scene_, atlas_loader_);
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);

    setWindowTitle(QString::fromStdString("Map Editor - " + config_path));
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

    auto* view_menu = menuBar()->addMenu("&View");
    auto* walkable_action = view_menu->addAction("Show &Walkable Overlay");
    walkable_action->setCheckable(true);
    walkable_action->setChecked(show_walkable_overlay_);
    connect(walkable_action, &QAction::triggered, this, &MainWindow::toggle_walkable_overlay);
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
    if (obj == view_->viewport() && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        auto click = MapInteraction::resolve_click(me, view_, doc_.tile_size(), doc_.height(),
                                                   doc_.width());
        if (!click.valid)
            return false;

        if (click.button == Qt::LeftButton && interaction_.has_selection()) {
            const auto& name = interaction_.selected();
            if (doc_.is_prop(name)) {
                doc_.set_prop(click.row, click.col, name);
                renderer_->update_prop(click.row, click.col, name, doc_);
            } else {
                doc_.set_tile(click.row, click.col, name);
                renderer_->update_tile(click.row, click.col, name, doc_, show_walkable_overlay_);
            }
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
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::save_map() {
    if (file_manager_->save(doc_)) {
        setWindowTitle(QString::fromStdString("Map Editor - " + doc_.path()));
        statusBar()->showMessage(QString("Saved to %1").arg(QString::fromStdString(doc_.path())),
                                 3000);
    }
}

void MainWindow::save_map_as() {
    if (file_manager_->save_as(doc_)) {
        setWindowTitle(QString::fromStdString("Map Editor - " + doc_.path()));
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

    setWindowTitle(QString::fromStdString("Map Editor - " + doc_.path()));
    statusBar()->showMessage(QString("Opened %1").arg(QString::fromStdString(doc_.path())), 3000);
}

void MainWindow::new_map() {
    auto result = show_new_map_dialog(this);
    if (!result.accepted)
        return;

    renderer_->clear_all();
    doc_.create_new(result.height, result.width, doc_.config());
    atlas_loader_.clear();
    atlas_loader_.load(doc_.config());

    rebuild_palette();
    full_rebuild();

    setWindowTitle("Map Editor - Untitled");
    statusBar()->showMessage(
            QString("New map: %1 x %2 tiles").arg(result.width).arg(result.height));
}

void MainWindow::resize_map(int cols, int rows) {
    renderer_->clear_all();
    doc_.resize(rows, cols, "");
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);

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

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (first_show_) {
        first_show_ = false;
        view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
}
