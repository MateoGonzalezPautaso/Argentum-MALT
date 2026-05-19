#ifndef EDITOR_TILE_PALETTE_H
#define EDITOR_TILE_PALETTE_H

#include <QWidget>
#include <QPixmap>
#include <QToolButton>
#include <string>
#include <unordered_map>
#include <vector>
#include "../common/config.h"

class QButtonGroup;

class TilePalette : public QWidget {
    Q_OBJECT

public:
    TilePalette(TilemapConfig& config,
                std::unordered_map<std::string, QPixmap>& atlases,
                QWidget* parent = nullptr);

    std::string selected_tile() const { return selected_tile_; }

signals:
    void tile_selected(const std::string& tile_name);
    void prop_selected(const std::string& prop_name);

private:
    struct SectionWidgets {
        QToolButton* header;
        QWidget* content;
    };

    SectionWidgets make_section(const std::string& title,
                                const std::vector<std::string>& tile_names,
                                int tsz, int preview_size);
    SectionWidgets make_prop_section(const std::string& title,
                                     const std::vector<std::string>& prop_names,
                                     int tsz, int preview_size);
    void on_button_clicked(const std::string& name);
    void toggle_walkable(const std::string& name);
    void update_button_visual(QToolButton* btn, const std::string& name) const;

    std::string selected_tile_;
    QButtonGroup* button_group_ = nullptr;
    TilemapConfig& config_;
    std::unordered_map<std::string, QPixmap>& atlases_;
    std::unordered_map<std::string, QToolButton*> tile_buttons_;
    std::vector<SectionWidgets> sections_;
};

#endif
