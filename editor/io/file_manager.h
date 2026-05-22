#ifndef EDITOR_FILE_MANAGER_H
#define EDITOR_FILE_MANAGER_H

#include <QWidget>
#include <string>
#include "../document/tilemap_document.h"

class FileManager {
public:
    explicit FileManager(QWidget* parent);

    bool save(TilemapDocument& doc);
    bool save_as(TilemapDocument& doc);
    bool open(TilemapDocument& doc);
    const std::string& last_error() const { return last_error_; }

private:
    QWidget* parent_;
    std::string last_error_;
};

#endif
