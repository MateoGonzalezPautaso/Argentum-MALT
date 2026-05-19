#ifndef EDITOR_MAIN_WINDOW_H
#define EDITOR_MAIN_WINDOW_H

#include <QMainWindow>
#include "tilemap_document.h"

class QGraphicsScene;
class QGraphicsView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const std::string& config_path, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setup_ui();
    void draw_grid();

    TilemapDocument doc_;
    QGraphicsScene* scene_ = nullptr;
    QGraphicsView* view_ = nullptr;
    bool first_show_ = true;
};

#endif
