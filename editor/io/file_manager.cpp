#include "file_manager.h"

#include <QFileDialog>
#include <QMessageBox>
#include <stdexcept>

FileManager::FileManager(QWidget* parent): parent_(parent) {}

bool FileManager::save(TilemapDocument& doc) {
    if (doc.path().empty()) {
        return save_as(doc);
    }
    try {
        doc.save(doc.path());
        return true;
    } catch (const std::exception& e) {
        QMessageBox::warning(parent_, "Save Error", e.what());
        return false;
    }
}

bool FileManager::save_as(TilemapDocument& doc) {
    QString path = QFileDialog::getSaveFileName(
            parent_, "Save Map As",
            QString::fromStdString(doc.path().empty() ? "config/common_tilemap.toml" : doc.path()),
            "TOML files (*.toml)");
    if (path.isEmpty())
        return false;

    try {
        doc.save(path.toStdString());
        doc.set_path(path.toStdString());
        return true;
    } catch (const std::exception& e) {
        QMessageBox::warning(parent_, "Save Error", e.what());
        return false;
    }
}

bool FileManager::open(TilemapDocument& doc) {
    QString path = QFileDialog::getOpenFileName(
            parent_, "Open Map", QString::fromStdString(doc.path().empty() ? "config" : doc.path()),
            "TOML files (*.toml)");
    if (path.isEmpty())
        return false;

    try {
        doc.load(path.toStdString());
        return true;
    } catch (const std::exception& e) {
        QMessageBox::critical(parent_, "Open Error", e.what());
        return false;
    }
}
