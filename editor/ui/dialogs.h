#ifndef EDITOR_DIALOGS_H
#define EDITOR_DIALOGS_H

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

#include "common/config.h"

constexpr int kMaxMapDimension = 256;

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

#endif
