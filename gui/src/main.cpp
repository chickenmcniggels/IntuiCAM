#include <QApplication>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QDebug>

#include "MainWindow.h"
#include "Core.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("IntuiCAM");
    app.setApplicationVersion("1.0.0");
    
    // Set up command line options
    QCommandLineParser parser;
    parser.setApplicationDescription("IntuiCAM - CAD/CAM Software");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add custom options
    QCommandLineOption fileOption(QStringList() << "f" << "file", 
                               "Open the specified file", "file");
    parser.addOption(fileOption);
    
    // Process the command line arguments
    parser.process(app);
    
    // Initialize the core engine
    IntuiCAM::Core core;
    if (!core.initialize()) {
        qCritical() << "Failed to initialize IntuiCAM core engine!";
        return 1;
    }
    
    // Optional: Display splash screen
    // QPixmap splashPixmap(":/images/splash.png");
    // QSplashScreen splash(splashPixmap);
    // splash.show();
    // app.processEvents();
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();
    
    // Hide splash screen
    // QTimer::singleShot(1500, &splash, &QSplashScreen::close);
    
    // Open file if specified
    if (parser.isSet(fileOption)) {
        QString filePath = parser.value(fileOption);
        QTimer::singleShot(500, [&mainWindow, filePath]() {
            mainWindow.openFile(filePath);
        });
    }
    
    // Run the application
    int result = app.exec();
    
    // Shutdown the core engine
    core.shutdown();
    
    return result;
} 