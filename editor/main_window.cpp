#include "main_window.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QShowEvent>
#include <QSplitter>
#include <QStatusBar>
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

    setup_ui();
    render_tiles();
    draw_grid();

    setWindowTitle(QString::fromStdString("Map Editor - " + config_path));
    resize(1200, 800);
}

void MainWindow::setup_ui() {
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    // Left: canvas
    auto* canvas_container = new QWidget();
    auto* canvas_layout = new QVBoxLayout(canvas_container);
    canvas_layout->setContentsMargins(0, 0, 0, 0);

    scene_ = new QGraphicsScene(this);
    view_ = new QGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing, false);
    view_->setDragMode(QGraphicsView::ScrollHandDrag);
    view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    canvas_layout->addWidget(view_);

    splitter->addWidget(canvas_container);

    // Right: palette placeholder
    auto* right_panel = new QWidget();
    auto* right_layout = new QVBoxLayout(right_panel);
    right_layout->addWidget(new QLabel("Tile Palette"));
    right_panel->setMinimumWidth(220);
    splitter->addWidget(right_panel);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    statusBar()->showMessage(
        QString("Map: %1 x %2 tiles  |  Tile size: %3 px")
            .arg(doc_.width()).arg(doc_.height()).arg(doc_.tile_size()));
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

void MainWindow::render_tiles() {
    const auto& cfg = doc_.config();
    if (cfg.path.empty()) return;

    atlas_ = QPixmap(QString::fromStdString(cfg.path));
    if (atlas_.isNull()) return;

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

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (first_show_) {
        first_show_ = false;
        view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
}
