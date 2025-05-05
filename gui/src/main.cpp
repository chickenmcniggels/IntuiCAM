#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include "IntuiCAMCore/Core.h"

int main(int argc, char *argv[]) {
    // 1) Initialize the Qt application
    QApplication app(argc, argv);                                                      // QApplication :contentReference[oaicite:3]{index=3}

    // 2) Create the main window
    QMainWindow window;                                                                 // QMainWindow :contentReference[oaicite:4]{index=4}
    window.setWindowTitle("IntuiCAM");

    // 3) Instantiate the CAM engine
    IntuiCAM::Core::CAMEngine engine;

    // 4) Build a File menu with an "Open STEP..." action
    QMenu* fileMenu = window.menuBar()->addMenu("&File");                               // QMenuBar:contentReference[oaicite:5]{index=5}
    QAction* openAction = fileMenu->addAction("Open &STEP...");

    // 5) Connect the action to a lambda that shows a file dialog
    QObject::connect(openAction, &QAction::triggered, [&window, &engine]() {
        QString fileName = QFileDialog::getOpenFileName(
            &window,
            "Open STEP File",
            "",
            "STEP Files (*.step *.stp)"                                               // QFileDialog filter :contentReference[oaicite:6]{index=6}
        );
        if (!fileName.isEmpty()) {
            bool ok = engine.loadSTEP(fileName.toStdString());
            QMessageBox::information(
                &window,
                "Import STEP",
                ok ? "STEP file loaded successfully." : "Failed to load STEP file."     // QMessageBox :contentReference[oaicite:7]{index=7}
            );
        }
    });

    // 6) Show the window
    window.resize(1024, 768);
    window.show();

    // 7) Enter the Qt event loop
    return app.exec();
}
