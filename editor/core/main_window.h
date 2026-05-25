#ifndef EDITOR_MAIN_WINDOW_H
#define EDITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include <string>
#include "../document/tilemap_document.h"
#include "../render/atlas_loader.h"
#include "../input/map_interaction.h"

class QGraphicsScene;
class QGraphicsView;
class QSplitter;
class QSpinBox;
class TilePalette;
class MapSceneRenderer;
class FileManager;

class MainWindow : public QMainWindow {
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
};

#endif
