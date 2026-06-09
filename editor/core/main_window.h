#ifndef EDITOR_MAIN_WINDOW_H
#define EDITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include <memory>
#include <string>

#include "../input/map_interaction.h"
#include "../ui/tile_palette.h"
#include "editor_controller.h"

class QAction;
class QActionGroup;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsView;
class QSplitter;
class QSpinBox;
class QLabel;

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
    void rebuild_palette();

    void save_map();
    void save_map_as();
    void open_map();
    void new_map();
    void resize_map(int cols, int rows);
    void update_title();
    void change_map_type(QAction* action);
    void update_drag_preview(int r1, int c1, int r2, int c2);
    void destroy_drag_preview();

    std::unique_ptr<EditorController> controller_;
    MapInteraction interaction_;

    QGraphicsScene* scene_ = nullptr;
    QGraphicsView* view_ = nullptr;
    QSplitter* splitter_ = nullptr;
    TilePalette* palette_ = nullptr;
    QSpinBox* width_spin_ = nullptr;
    QSpinBox* height_spin_ = nullptr;

    QActionGroup* map_type_group_ = nullptr;
    QAction* spawn_zone_mode_action_ = nullptr;
    QLabel* zoom_label_ = nullptr;
    double zoom_level_ = 1.0;

    bool first_show_ = true;
    bool spawn_zone_mode_ = false;
    bool dragging_ = false;
    int drag_start_row_ = -1;
    int drag_start_col_ = -1;
    QGraphicsRectItem* drag_preview_ = nullptr;
};

#endif
