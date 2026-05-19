#ifndef EDITOR_MAIN_WINDOW_H
#define EDITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <string>
#include "tilemap_document.h"

class QGraphicsScene;
class QGraphicsView;
class QSplitter;
class QSpinBox;
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
    void render_props();
    void set_tile(int row, int col, const std::string& name);
    void set_prop(int row, int col, const std::string& name);
    void resize_map(int cols, int rows);
    void save_map();
    void save_map_as();
    void new_map();
    void open_map();
    void toggle_walkable_overlay();
    void clear_grid(std::vector<std::vector<QGraphicsPixmapItem*>>& grid);
    void connect_palette_signals();

    TilemapDocument doc_;
    std::unordered_map<std::string, QPixmap> atlases_;
    std::vector<std::vector<QGraphicsPixmapItem*>> tile_items_;
    std::vector<std::vector<QGraphicsPixmapItem*>> prop_items_;
    QGraphicsScene* scene_ = nullptr;
    QGraphicsView* view_ = nullptr;
    QSplitter* splitter_ = nullptr;
    TilePalette* palette_ = nullptr;
    QSpinBox* width_spin_ = nullptr;
    QSpinBox* height_spin_ = nullptr;
    std::string selected_tile_;
    bool first_show_ = true;
    bool show_walkable_overlay_ = true;
};

#endif
