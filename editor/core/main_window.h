#ifndef EDITOR_MAIN_WINDOW_H
#define EDITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include <string>

#include "../document/tilemap_document.h"
#include "../input/map_interaction.h"
#include "../render/atlas_loader.h"

class QAction;
class QActionGroup;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsView;
class QSplitter;
class QSpinBox;
class TilePalette;
class MapSceneRenderer;
class FileManager;

class MainWindow: public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const std::string& config_path, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setup_ui();
    void connect_palette_signals();
    void full_rebuild();
    void rebuild_palette();

    void save_map();
    void save_map_as();
    void open_map();
    void new_map();
    void resize_map(int cols, int rows);
    void toggle_walkable_overlay();

    void place_tile_or_prop(int row, int col, const std::string& name);
    void fill_rect(int r1, int c1, int r2, int c2, const std::string& name);
    void fill_spawn_zone_rect(int r1, int c1, int r2, int c2);
    void update_drag_preview(int r1, int c1, int r2, int c2);
    void destroy_drag_preview();

    void update_title();
    void change_map_type(QAction* action);
    void toggle_spawn_zone_mode();
    void toggle_spawn_overlay();
    bool validate_city_map() const;

    TilemapConfig default_tile_config_;
    TilemapDocument doc_;
    AtlasLoader atlas_loader_;
    MapInteraction interaction_;
    MapSceneRenderer* renderer_ = nullptr;
    FileManager* file_manager_ = nullptr;

    QGraphicsScene* scene_ = nullptr;
    QGraphicsView* view_ = nullptr;
    QSplitter* splitter_ = nullptr;
    TilePalette* palette_ = nullptr;
    QSpinBox* width_spin_ = nullptr;
    QSpinBox* height_spin_ = nullptr;

    bool first_show_ = true;
    bool show_walkable_overlay_ = true;

    QActionGroup* map_type_group_ = nullptr;

    QAction* spawn_zone_mode_action_ = nullptr;
    bool spawn_zone_mode_ = false;

    bool dragging_ = false;
    int drag_start_row_ = -1;
    int drag_start_col_ = -1;
    QGraphicsRectItem* drag_preview_ = nullptr;
};

#endif
