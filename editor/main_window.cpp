#include "main_window.h"
#include "tile_palette.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(const std::string& config_path, QWidget* parent)
    : QMainWindow(parent) {
    try {
        doc_.load(config_path);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Load Error",
            QString("Failed to load %1:\n%2")
                .arg(QString::fromStdString(config_path), e.what()));
        QApplication::quit();
        return;
    }

    load_atlas();
    setup_ui();
    render_tiles();
    draw_grid();

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

    // Left: canvas
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

    // Right: tile palette
    palette_ = new TilePalette(doc_.config(), atlas_, this);
    splitter_->addWidget(palette_);

    connect(palette_, &TilePalette::tile_selected, this,
            [this](const std::string& name) {
                selected_tile_ = name;
                statusBar()->showMessage(
                    QString("Selected tile: %1  |  Map: %2 x %3")
                        .arg(QString::fromStdString(name))
                        .arg(doc_.width()).arg(doc_.height()));
            });

    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 1);

    statusBar()->showMessage(
        QString("Map: %1 x %2 tiles  |  Tile size: %3 px")
            .arg(doc_.width()).arg(doc_.height()).arg(doc_.tile_size()));

    // Toolbar: resize controls
    auto* toolbar = addToolBar("Map");
    toolbar->setMovable(false);

    auto* width_label = new QLabel("Width:");
    width_spin_ = new QSpinBox();
    width_spin_->setRange(1, 256);
    width_spin_->setValue(doc_.width());

    auto* height_label = new QLabel("  Height:");
    height_spin_ = new QSpinBox();
    height_spin_->setRange(1, 256);
    height_spin_->setValue(doc_.height());

    auto* resize_btn = new QPushButton("Resize");

    toolbar->addWidget(width_label);
    toolbar->addWidget(width_spin_);
    toolbar->addWidget(height_label);
    toolbar->addWidget(height_spin_);
    toolbar->addWidget(resize_btn);

    connect(resize_btn, &QPushButton::clicked, this, [this]() {
        resize_map(width_spin_->value(), height_spin_->value());
    });

    // File menu
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
}

void MainWindow::draw_grid() {
    auto rows = doc_.height();
    auto cols = doc_.width();
    auto tsz = doc_.tile_size();

    int w = cols * tsz;
    int h = rows * tsz;

    QPen grid_pen(QColor(80, 80, 80), 1);

    for (int r = 0; r <= rows; ++r) {
        auto* line = scene_->addLine(0, r * tsz, w, r * tsz, grid_pen);
        line->setZValue(1);
    }
    for (int c = 0; c <= cols; ++c) {
        auto* line = scene_->addLine(c * tsz, 0, c * tsz, h, grid_pen);
        line->setZValue(1);
    }

    scene_->setSceneRect(0, 0, w, h);
}

void MainWindow::load_atlas() {
    const auto& cfg = doc_.config();
    if (!cfg.path.empty()) {
        atlas_ = QPixmap(QString::fromStdString(cfg.path));
    }
}

void MainWindow::render_tiles() {
    if (atlas_.isNull()) return;

    const auto& cfg = doc_.config();

    auto tsz = doc_.tile_size();
    tile_items_.reserve(doc_.height());

    for (int r = 0; r < doc_.height(); ++r) {
        std::vector<QGraphicsPixmapItem*> row_items;
        row_items.reserve(doc_.width());

        for (int c = 0; c < doc_.width(); ++c) {
            const auto& name = doc_.tile_name(r, c);
            auto it = cfg.tiles.find(name);
            if (it == cfg.tiles.end()) {
                row_items.push_back(nullptr);
                continue;
            }

            const auto& def = it->second;
            QPixmap tile = atlas_.copy(QRect(def.x, def.y, tsz, tsz));
            auto* item = scene_->addPixmap(tile);
            item->setPos(c * tsz, r * tsz);

            row_items.push_back(item);
        }

        tile_items_.push_back(std::move(row_items));
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == view_->viewport() && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPointF scene_pos = view_->mapToScene(me->pos());

        auto tsz = doc_.tile_size();
        int col = static_cast<int>(scene_pos.x()) / tsz;
        int row = static_cast<int>(scene_pos.y()) / tsz;

        if (row < 0 || row >= doc_.height() || col < 0 || col >= doc_.width())
            return false;

        if (me->button() == Qt::LeftButton && !selected_tile_.empty()) {
            set_tile(row, col, selected_tile_);
            return true;
        }
        if (me->button() == Qt::RightButton) {
            set_tile(row, col, "");
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::set_tile(int row, int col, const std::string& name) {
    doc_.set_tile(row, col, name);

    auto& old_item = tile_items_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    if (old_item) {
        scene_->removeItem(old_item);
        delete old_item;
        old_item = nullptr;
    }

    if (!name.empty() && !atlas_.isNull()) {
        auto it = doc_.config().tiles.find(name);
        if (it != doc_.config().tiles.end()) {
            const auto& def = it->second;
            auto tsz = doc_.tile_size();
            QPixmap tile = atlas_.copy(QRect(def.x, def.y, tsz, tsz));
            auto* item = scene_->addPixmap(tile);
            item->setPos(col * tsz, row * tsz);
            tile_items_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = item;
        }
    }
}

void MainWindow::resize_map(int new_width, int new_height) {
    for (auto& row : tile_items_) {
        for (auto* item : row) {
            if (item) {
                scene_->removeItem(item);
                delete item;
            }
        }
    }
    tile_items_.clear();

    scene_->clear();

    doc_.resize(new_height, new_width, "");

    render_tiles();
    draw_grid();

    width_spin_->setValue(doc_.width());
    height_spin_->setValue(doc_.height());

    view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);

    statusBar()->showMessage(
        QString("Map resized to %1 x %2 tiles")
            .arg(doc_.width()).arg(doc_.height()));
}

void MainWindow::save_map() {
    if (doc_.path().empty()) {
        save_map_as();
        return;
    }
    try {
        doc_.save(doc_.path());
        statusBar()->showMessage(
            QString("Saved to %1").arg(QString::fromStdString(doc_.path())), 3000);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Save Error", e.what());
    }
}

void MainWindow::save_map_as() {
    QString path = QFileDialog::getSaveFileName(this, "Save Map As",
        QString::fromStdString(doc_.path().empty()
            ? "config/common_tilemap.toml" : doc_.path()),
        "TOML files (*.toml)");
    if (path.isEmpty()) return;

    try {
        doc_.save(path.toStdString());
        doc_.set_path(path.toStdString());
        setWindowTitle(QString::fromStdString("Map Editor - " + path.toStdString()));
        statusBar()->showMessage(QString("Saved to %1").arg(path), 3000);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Save Error", e.what());
    }
}

void MainWindow::open_map() {
    QString path = QFileDialog::getOpenFileName(this, "Open Map",
        QString::fromStdString(doc_.path().empty()
            ? "config" : doc_.path()),
        "TOML files (*.toml)");
    if (path.isEmpty()) return;

    try {
        for (auto& row : tile_items_) {
            for (auto* item : row) {
                if (item) {
                    scene_->removeItem(item);
                    delete item;
                }
            }
        }
        tile_items_.clear();
        scene_->clear();

        doc_.load(path.toStdString());
        load_atlas();

        // Rebuild palette
        delete palette_;
        palette_ = new TilePalette(doc_.config(), atlas_, this);
        splitter_->addWidget(palette_);
        connect(palette_, &TilePalette::tile_selected, this,
                [this](const std::string& name) {
                    selected_tile_ = name;
                    statusBar()->showMessage(
                        QString("Selected tile: %1  |  Map: %2 x %3")
                            .arg(QString::fromStdString(name))
                            .arg(doc_.width()).arg(doc_.height()));
                });

        render_tiles();
        draw_grid();

        width_spin_->setValue(doc_.width());
        height_spin_->setValue(doc_.height());

        view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);

        setWindowTitle(QString::fromStdString("Map Editor - " + path.toStdString()));
        statusBar()->showMessage(QString("Opened %1").arg(path), 3000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Open Error", e.what());
    }
}

void MainWindow::new_map() {
    QDialog dialog(this);
    dialog.setWindowTitle("New Map");

    auto* form = new QFormLayout(&dialog);

    auto* w_spin = new QSpinBox();
    w_spin->setRange(1, 256);
    w_spin->setValue(20);

    auto* h_spin = new QSpinBox();
    h_spin->setRange(1, 256);
    h_spin->setValue(20);

    form->addRow("Width:", w_spin);
    form->addRow("Height:", h_spin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    int new_w = w_spin->value();
    int new_h = h_spin->value();

    for (auto& row : tile_items_) {
        for (auto* item : row) {
            if (item) {
                scene_->removeItem(item);
                delete item;
            }
        }
    }
    tile_items_.clear();
    scene_->clear();

    doc_.create_new(new_h, new_w, doc_.config());
    load_atlas();

    render_tiles();
    draw_grid();

    width_spin_->setValue(doc_.width());
    height_spin_->setValue(doc_.height());

    view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);

    setWindowTitle("Map Editor - Untitled");
    statusBar()->showMessage(
        QString("New map: %1 x %2 tiles").arg(new_w).arg(new_h));
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (first_show_) {
        first_show_ = false;
        view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
}
