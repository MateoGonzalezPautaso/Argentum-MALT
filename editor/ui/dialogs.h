#ifndef EDITOR_DIALOGS_H
#define EDITOR_DIALOGS_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

struct NewMapResult {
    int width = 20;
    int height = 20;
    bool accepted = false;
};

inline NewMapResult show_new_map_dialog(QWidget* parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle("New Map");

    auto* form = new QFormLayout(&dialog);

    auto* w_spin = new QSpinBox();
    w_spin->setRange(1, 256);
    w_spin->setValue(20);

    auto* h_spin = new QSpinBox();
    h_spin->setRange(1, 256);
    h_spin->setValue(20);

    form->addRow("Width:", w_spin);
    form->addRow("Height:", h_spin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    NewMapResult result;
    if (dialog.exec() == QDialog::Accepted) {
        result.width = w_spin->value();
        result.height = h_spin->value();
        result.accepted = true;
    }
    return result;
}

#endif
