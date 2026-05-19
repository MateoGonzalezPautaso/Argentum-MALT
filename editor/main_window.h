#ifndef EDITOR_MAIN_WINDOW_H
#define EDITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include "tilemap_document.h"

class QGraphicsScene;
class QGraphicsView;
class TilePalette;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const std::string& config_path, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setup_ui();
    void load_atlas();
    void draw_grid();
    void render_tiles();
    void set_tile(int row, int col, const std::string& name);

    TilemapDocument doc_;
    QPixmap atlas_;
    std::vector<std::vector<QGraphicsPixmapItem*>> tile_items_;
    QGraphicsScene* scene_ = nullptr;
    QGraphicsView* view_ = nullptr;
    TilePalette* palette_ = nullptr;
    std::string selected_tile_;
    bool first_show_ = true;
};

#endif
