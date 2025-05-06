#include <QApplication>
#include "./../include/MainWIndow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    IntuiCAM::GUI::IntuiCAMMainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
