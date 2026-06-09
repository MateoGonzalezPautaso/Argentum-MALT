#ifndef EDITOR_DIALOGS_H
#define EDITOR_DIALOGS_H

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

#include <climits>

#include <toml++/toml.hpp>

#include "common/config.h"

constexpr int kMaxMapDimension = INT_MAX;

struct NewMapResult {
    int width = 20;
    int height = 20;
    MapType map_type = MapType::NONE;
    bool accepted = false;
};

inline NewMapResult show_new_map_dialog(QWidget* parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle("New Map");

    auto* form = new QFormLayout(&dialog);

    auto* w_spin = new QSpinBox();
    w_spin->setRange(1, kMaxMapDimension);
    w_spin->setValue(20);

    auto* h_spin = new QSpinBox();
    h_spin->setRange(1, kMaxMapDimension);
    h_spin->setValue(20);

    auto* type_combo = new QComboBox();
    type_combo->addItem("None", static_cast<int>(MapType::NONE));
    type_combo->addItem("City", static_cast<int>(MapType::CITY));
    type_combo->addItem("Dungeon", static_cast<int>(MapType::DUNGEON));

    form->addRow("Width:", w_spin);
    form->addRow("Height:", h_spin);
    form->addRow("Type:", type_combo);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    NewMapResult result;
    if (dialog.exec() == QDialog::Accepted) {
        result.width = w_spin->value();
        result.height = h_spin->value();
        result.map_type = static_cast<MapType>(type_combo->currentData().toInt());
        result.accepted = true;
    }
    return result;
}

struct TransitionResult {
    std::string transition_map;
    int transition_x = 0;
    int transition_y = 0;
    bool accepted = false;
};

inline std::string basename(const std::string& path) {
    auto pos = path.rfind('/');
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

inline TransitionResult show_transition_dialog(QWidget* parent,
                                                const std::string& current_map,
                                                int current_x, int current_y) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Configure Portal");

    auto* form = new QFormLayout(&dialog);

    auto* map_combo = new QComboBox();
    map_combo->setEditable(true);
    try {
        toml::table map_list = toml::parse_file("config/map_list.toml");
        if (auto maps_arr = map_list["maps"].as_array()) {
            for (const auto& entry : *maps_arr) {
                if (auto tbl = entry.as_table()) {
                    if (auto name = (*tbl)["name"].value<std::string>()) {
                        if (auto path = (*tbl)["path"].value<std::string>()) {
                            QString display = QString::fromStdString(basename(*path));
                            map_combo->addItem(display, QString::fromStdString(*name));
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        qWarning("Failed to load map_list.toml: %s", e.what());
    }

    int idx = map_combo->findData(QString::fromStdString(current_map));
    if (idx >= 0) {
        map_combo->setCurrentIndex(idx);
    } else if (!current_map.empty()) {
        map_combo->setCurrentText(QString::fromStdString(current_map));
    }

    auto* x_spin = new QSpinBox();
    x_spin->setRange(0, 9999);
    x_spin->setValue(current_x);

    auto* y_spin = new QSpinBox();
    y_spin->setRange(0, 9999);
    y_spin->setValue(current_y);

    auto* clear_label = new QLabel(
        "Leave \"Destination Map\" empty to remove the portal.");
    clear_label->setWordWrap(true);
    clear_label->setStyleSheet("color: #888; font-size: 11px;");

    form->addRow("Destination Map:", map_combo);
    form->addRow(clear_label);
    form->addRow("Spawn X:", x_spin);
    form->addRow("Spawn Y:", y_spin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    TransitionResult result;
    if (dialog.exec() == QDialog::Accepted) {
        int cur_idx = map_combo->currentIndex();
        if (cur_idx >= 0 && map_combo->itemData(cur_idx).isValid()) {
            result.transition_map = map_combo->itemData(cur_idx).toString().toStdString();
        } else {
            result.transition_map = map_combo->currentText().toStdString();
            auto dot = result.transition_map.rfind(".toml");
            if (dot != std::string::npos)
                result.transition_map = result.transition_map.substr(0, dot);
        }
        result.transition_x = x_spin->value();
        result.transition_y = y_spin->value();
        result.accepted = true;
    }
    return result;
}

#endif
