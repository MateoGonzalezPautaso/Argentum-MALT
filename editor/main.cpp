#include <QApplication>
#include "main_window.h"

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "xcb");
    QApplication app(argc, argv);

    std::string config_path = "config/common_tilemap.toml";
    if (argc > 1) {
        config_path = argv[1];
    }

    MainWindow window(config_path);
    window.show();

    return app.exec();
}
